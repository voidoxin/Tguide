#include "DBResolver.h"
#include "ErrorHandler.h"
#include "db_cache_manager.h"
#include "sha256.h"
#include <iostream>
#include <filesystem>
#include <functional>
#include <set>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sqlite3.h>
#include <curl/curl.h>

DBResolver& DBResolver::instance() {
    static DBResolver r;
    return r;
}

#ifndef NDEBUG
void DBResolver::resetForTesting() {
    fatal_ = false;
    cacheValidated_ = false;
    resolvedCache_.clear();
}
#endif

// ==================== PATH HELPERS ====================

static bool isSafePath(const std::string& s) {
    for (char c : s) {
        if (c == ';' || c == '|' ||
            c == '`' || c == '$' || c == '\n' || c == '\r')
            return false;
    }
    if (s.find("..") != std::string::npos) return false;
    return true;
}

static bool isSafeURL(const std::string& s) {
    for (char c : s) {
        if (c == ';' || c == '|' ||
            c == '`' || c == '$' || c == '\n' || c == '\r')
            return false;
    }
    return true;
}

static const std::string DEFAULT_DB_PATH = "data/database/tguide.db";

// ==================== VALIDATION ====================

static bool isSQLiteFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return false;
    uint8_t header[16];
    f.read(reinterpret_cast<char*>(header), 16);
    if (f.gcount() != 16) return false;
    static const uint8_t MAGIC[16] = {
        'S','Q','L','i','t','e',' ','f','o','r','m','a','t',' ','3',0x00
    };
    return memcmp(header, MAGIC, 16) == 0;
}

static bool validateSchema(sqlite3* db) {
    struct TableSchema {
        const char*              table;
        std::vector<const char*> columns;
    };

    const std::vector<TableSchema> schemas = {
        { "vulnerabilities", { "id","name","metasploit_name","discovered_date","discoverer",
                                "severity","access","platform","service","description","danger" } },
        { "options",         { "id","vuln_id","option_name","option_value" } },
        { "modules",         { "id","name","path","platform","type","description",
                                "API","mode","loud","output" } },
        { "tools",           { "id","name","category","short_desc","description","flags_all" } },
        { "tool_flags",      { "id","tool_id","name","description","loud","root","protocols" } },
        { "templates",       { "id","tool_id","template_name","description",
                                "root","protocols","flag" } }
    };

    for (auto& schema : schemas) {
        const char* checkSql =
            "SELECT name FROM sqlite_master WHERE type='table' AND name=?";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, checkSql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        sqlite3_bind_text(stmt, 1, schema.table, -1, SQLITE_TRANSIENT);
        bool exists = sqlite3_step(stmt) == SQLITE_ROW;
        sqlite3_finalize(stmt);
        if (!exists) return false;

        std::string pragma = std::string("PRAGMA table_info(") + schema.table + ")";
        if (sqlite3_prepare_v2(db, pragma.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            return false;
        std::set<std::string> actualCols;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            const char* col =
                reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            if (col) actualCols.insert(col);
        }
        sqlite3_finalize(stmt);

        for (auto& col : schema.columns) {
            if (actualCols.find(col) == actualCols.end()) return false;
        }
    }
    return true;
}

static bool dbHasData(sqlite3* db) {
    const char* tables[] = {
        "vulnerabilities", "options", "modules",
        "tools", "tool_flags", "templates"
    };
    for (auto& table : tables) {
        std::string sql = std::string("SELECT COUNT(*) FROM ") + table;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            continue;
        bool hasRows = sqlite3_step(stmt) == SQLITE_ROW &&
                       sqlite3_column_int(stmt, 0) > 0;
        sqlite3_finalize(stmt);
        if (hasRows) return true;
    }
    return false;
}

// ==================== DOWNLOAD ====================

static size_t curlWriteCallback(void* ptr, size_t size,
                                 size_t nmemb, void* stream) {
    return fwrite(ptr, size, nmemb, static_cast<FILE*>(stream));
}

bool DBResolver::downloadDB(const std::string& url, const std::string& destPath) {
    if (url.empty())            return false;
    if (!isSafeURL(url))        return false;
    if (!isSafePath(destPath))  return false;

    if (url.rfind("https://", 0) != 0) return false;

    FILE* f = fopen(destPath.c_str(), "wb");
    if (!f) return false;

    CURL* curl = curl_easy_init();
    if (!curl) {
        fclose(f);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS,       5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      f);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR,    1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE,    20971520L);

    // --- SSL/TLS security options ---
    // CA bundle path is intentionally not set — we rely on libcurl's
    // compiled-in default, which is correct per target platform.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,  1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,  2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION,      CURL_SSLVERSION_TLSv1_2);
#if LIBCURL_VERSION_NUM >= 0x075500  // curl >= 7.85.0: _STR variants
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR,       "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
#else
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS,        CURLPROTO_HTTPS);
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS,  CURLPROTO_HTTPS);
#endif

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(f);

    if (res != CURLE_OK) {
        std::error_code ec;
        std::filesystem::remove(destPath, ec);

        // Distinguish common SSL failures from generic download errors
        if (res == CURLE_SSL_CONNECT_ERROR ||
            res == CURLE_PEER_FAILED_VERIFICATION ||
            res == CURLE_SSL_CERTPROBLEM) {
            if (g_errorHandler.error)
                g_errorHandler.error(
                    "SSL/TLS verification failed during database download. "
                    "Check your system date, CA certificates, or network.");
        }

        return false;
    }

    if (!isSQLiteFile(destPath)) {
        std::error_code ec;
        std::filesystem::remove(destPath, ec);
        return false;
    }

    return true;
}

// ==================== RESOLVER ====================

void DBResolver::invalidateCacheIfMissing() {
    if (cacheValidated_) return;
    for (auto it = resolvedCache_.begin(); it != resolvedCache_.end(); ) {
        if (!std::filesystem::exists(it->second))
            it = resolvedCache_.erase(it);
        else
            ++it;
    }
    cacheValidated_ = true;
}

std::string DBResolver::cacheResult(const std::string& configPath,
                                     const std::string& resolvedPath,
                                     const std::string& hash) {
    DBCacheManager::instance().setCurrentHash(hash);
    DBCacheManager::instance().recordAccess(configPath, hash);
    DBCacheManager::instance().save();
    resolvedCache_[configPath] = resolvedPath;
    return resolvedPath;
}

bool DBResolver::openAndValidate(const std::string& path, bool& isSQLite,
                                  bool& schemaOk, bool& hasData,
                                  std::string& hash) {
    bool fileExists = std::filesystem::exists(path);
    isSQLite = fileExists && isSQLiteFile(path);
    schemaOk = false;
    hasData  = false;
    hash.clear();
    if (isSQLite) {
        sqlite3* db = nullptr;
        if (sqlite3_open(path.c_str(), &db) == SQLITE_OK) {
            schemaOk = validateSchema(db);
            hasData  = schemaOk && dbHasData(db);
            sqlite3_close(db);
        }
        hash = SHA256::hashFile(path);
    }
    return fileExists;
}

std::string DBResolver::copyDefaultToConfig(const std::string& configPath) {
    if (!std::filesystem::exists(DEFAULT_DB_PATH) ||
        !isSQLiteFile(DEFAULT_DB_PATH))
        return {};

    sqlite3* db = nullptr;
    bool defaultOk = false;
    if (sqlite3_open(DEFAULT_DB_PATH.c_str(), &db) == SQLITE_OK) {
        defaultOk = validateSchema(db);
        sqlite3_close(db);
    }
    if (!defaultOk) return {};

    std::error_code ec;
    std::filesystem::rename(DEFAULT_DB_PATH, configPath, ec);
    if (ec) {
        std::filesystem::copy_file(DEFAULT_DB_PATH, configPath,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (!ec) std::filesystem::remove(DEFAULT_DB_PATH);
    }
    if (ec) {
        if (g_errorHandler.error) g_errorHandler.error(
            "Failed to move default database to configured path.");
        return {};
    }
    std::string movedHash = SHA256::hashFile(configPath);
    return cacheResult(configPath, configPath, movedHash);
}

std::string DBResolver::resolveHashMismatch(const std::string& configPath,
                                             const std::string& hash,
                                             bool officialHashSet) {
    if (!std::filesystem::exists(DEFAULT_DB_PATH) ||
        !isSQLiteFile(DEFAULT_DB_PATH))
        return {};

    std::string defaultHash = SHA256::hashFile(DEFAULT_DB_PATH);
    if (!officialHashSet || defaultHash != DB_OFFICIAL_HASH)
        return {};

    std::error_code ec;
    std::filesystem::copy_file(DEFAULT_DB_PATH, configPath,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        if (g_errorHandler.error) g_errorHandler.error(
            "Failed to copy official database. Attempting recovery.");
        return {};
    }
    std::string newHash = SHA256::hashFile(configPath);
    return cacheResult(configPath, configPath, newHash);
}

std::string DBResolver::resolve(const std::string& configPath) {
    if (fatal_) return "";

    invalidateCacheIfMissing();

    // TOCTOU: re-validate cached path before returning
    {
        auto it = resolvedCache_.find(configPath);
        if (it != resolvedCache_.end()) {
            if (std::filesystem::exists(it->second))
                return it->second;
            resolvedCache_.erase(it);
        }
    }

    std::filesystem::create_directories(
        std::filesystem::path(configPath).parent_path()
    );

    bool        isSQLite, schemaOk, hasData;
    std::string hash;
    bool fileExists = openAndValidate(configPath, isSQLite, schemaOk,
                                       hasData, hash);

    const bool officialHashSet = !std::string(DB_OFFICIAL_HASH).empty();
    const bool isOfficial      = officialHashSet && hash == DB_OFFICIAL_HASH;

    // Step 1 — existing file is valid → use it
    if (fileExists && isSQLite && schemaOk && (isOfficial || !officialHashSet))
        return cacheResult(configPath, configPath, hash);

    // Step 2 — hash mismatch with valid data → ask user
    if (fileExists && isSQLite && schemaOk && hasData &&
        officialHashSet && !isOfficial) {
        char response = g_errorHandler.attention ? g_errorHandler.attention(
            "The database at the configured path does not match the official release. "
            "Enter 'y' to revert to the official database, or 'n' to keep the external one."
        ) : 0;

        if (response == 0 || response == 'n' || response == 'N')
            return cacheResult(configPath, configPath, hash);

        std::string result = resolveHashMismatch(configPath, hash,
                                                  officialHashSet);
        if (!result.empty()) return result;
        fileExists = false;  // file consumed — fall through
    }

    // Step 3 — try default DB
    {
        std::string result = copyDefaultToConfig(configPath);
        if (!result.empty()) return result;
    }

    // Step 4 — try download
    {
        std::string downloadUrl = std::string(DB_DOWNLOAD_URL);
        if (!downloadUrl.empty()) {
            std::cout << "  database not found \u2014 attempting download from GitHub...\n"
                      << std::flush;

            if (downloadDB(downloadUrl, configPath)) {
                sqlite3* db   = nullptr;
                bool     dlOk = false;
                if (sqlite3_open(configPath.c_str(), &db) == SQLITE_OK) {
                    dlOk = validateSchema(db);
                    sqlite3_close(db);
                }

                if (dlOk) {
                    std::string dlHash = SHA256::hashFile(configPath);
                    if (officialHashSet && dlHash != DB_OFFICIAL_HASH) {
                        std::error_code ec;
                        std::filesystem::remove(configPath, ec);
                        if (g_errorHandler.fatal) g_errorHandler.fatal(
                            "Downloaded database hash does not match the official release. "
                            "The file may have been tampered with. Aborting."
                        );
                        fatal_ = true;
                        return "";
                    }
                    return cacheResult(configPath, configPath, dlHash);
                }

                std::error_code ec;
                std::filesystem::remove(configPath, ec);
            }

            if (g_errorHandler.fatal) g_errorHandler.fatal(
                "Failed to download or validate the official database. "
                "Please check your internet connection or verify "
                "DB_DOWNLOAD_URL in db_cache_manager.h."
            );
        } else {
            if (g_errorHandler.fatal) g_errorHandler.fatal(
                "Database not found and DB_DOWNLOAD_URL is not configured. "
                "Set DB_DOWNLOAD_URL in db_cache_manager.h before release."
            );
        }
    }

    fatal_ = true;
    return "";
}
