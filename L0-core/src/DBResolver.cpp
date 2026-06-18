#include "DBResolver.h"
#include "ErrorHandler.h"
#include "db_cache_manager.h"
#include "sha256.h"
#include "path_resolver.h"
#include "../../libs/json.hpp"
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
    manifest_ = Manifest{};
    manifestFetched_ = false;
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
                                "root","protocols","flag" } },
        { "categories",      { "id","name","display_order","description" } }
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
        "tools", "tool_flags", "templates", "categories"
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

// ==================== MANIFEST ====================

static constexpr const char* MANIFEST_URL =
    "https://github.com/voidoxin/Tguide/releases/latest/download/signed_manifest.json";

static size_t curlStringCallback(void* ptr, size_t size,
                                  size_t nmemb, void* str) {
    std::string* s = static_cast<std::string*>(str);
    s->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}

DBResolver::Manifest DBResolver::fetchManifest() {
    if (manifestFetched_)
        return manifest_;

    CURL* curl = curl_easy_init();
    if (!curl) {
        if (g_errorHandler.error)
            g_errorHandler.error("Failed to initialize curl for manifest fetch.");
        return {};
    }

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL,            MANIFEST_URL);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS,       5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlStringCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR,    1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE,    1048576L);  // 1MB max for manifest

    // --- SSL/TLS security options ---
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

    if (res != CURLE_OK) {
        if (g_errorHandler.error)
            g_errorHandler.error(
                "Failed to fetch version manifest from GitHub. "
                "Check your internet connection. Using local database.");
        return {};
    }

    try {
        json manifestJson = json::parse(response);

        Manifest m;
        m.version = manifestJson.value("version", "");
        m.db_hash = manifestJson.value("db_hash", "");
        m.db_url  = manifestJson.value("db_url", "");

        // Validate required fields
        if (m.version.empty() || m.db_hash.empty() || m.db_url.empty()) {
            if (g_errorHandler.error)
                g_errorHandler.error(
                    "Version manifest is missing required fields.");
            return {};
        }

        // Validate db_hash is a 64-char hex string
        if (m.db_hash.size() != 64) {
            if (g_errorHandler.error)
                g_errorHandler.error(
                    "Version manifest contains an invalid db_hash.");
            return {};
        }
        for (char c : m.db_hash) {
            if (!((c >= '0' && c <= '9') ||
                  (c >= 'a' && c <= 'f') ||
                  (c >= 'A' && c <= 'F'))) {
                if (g_errorHandler.error)
                    g_errorHandler.error(
                        "Version manifest contains an invalid db_hash.");
                return {};
            }
        }

        // Validate db_url starts with https://
        if (m.db_url.rfind("https://", 0) != 0) {
            if (g_errorHandler.error)
                g_errorHandler.error(
                    "Version manifest contains a non-HTTPS db_url.");
            return {};
        }

        manifestFetched_ = true;
        manifest_ = m;

        // Persist version for future reference
        DBCacheManager::instance().setLastSeenVersion(m.version);
        DBCacheManager::instance().save();

        return m;

    } catch (const json::parse_error&) {
        if (g_errorHandler.error)
            g_errorHandler.error(
                "Failed to parse version manifest JSON.");
    } catch (const std::exception& e) {
        if (g_errorHandler.error)
            g_errorHandler.error(
                std::string("Unexpected error parsing manifest: ") + e.what());
    }

    return {};
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

// ==================== BACKUP / RESTORE ====================

bool DBResolver::tryRestoreFromBackup(const std::string& bakPath,
                                       const std::string& destPath) {
    if (!std::filesystem::exists(bakPath))
        return false;
    if (!isSQLiteFile(bakPath))
        return false;

    // Validate backup schema
    sqlite3* db = nullptr;
    bool valid = false;
    if (sqlite3_open(bakPath.c_str(), &db) == SQLITE_OK) {
        valid = validateSchema(db);
        sqlite3_close(db);
    }
    if (!valid)
        return false;

    // Verify backup hash matches the recorded hash
    std::string actualHash = SHA256::hashFile(bakPath);
    if (actualHash != DBCacheManager::instance().getBackupHash())
        return false;

    // Restore: copy backup over destination
    std::error_code ec;
    std::filesystem::copy_file(bakPath, destPath,
        std::filesystem::copy_options::overwrite_existing, ec);
    return !ec;
}

// ==================== DATABASE INFO ====================

DBResolver::DbInfo DBResolver::getDatabaseInfo(const std::string& dbPath) {
    DbInfo info{0, 0, 0};

    // File size
    std::error_code ec;
    info.fileSize = std::filesystem::file_size(dbPath, ec);
    if (ec) return info;

    // Open DB and query metadata
    sqlite3* db = nullptr;
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) return info;

    // Count user tables (exclude sqlite_* internal tables)
    {
        const char* sql = "SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'";
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            if (sqlite3_step(stmt) == SQLITE_ROW)
                info.tableCount = sqlite3_column_int(stmt, 0);
            sqlite3_finalize(stmt);
        }
    }

    // Count total rows across all user tables
    {
        // Get table names first
        const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%'";
        sqlite3_stmt* stmt = nullptr;
        std::vector<std::string> tables;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                if (name) tables.push_back(name);
            }
            sqlite3_finalize(stmt);
        }

        for (const auto& t : tables) {
            char* escaped = sqlite3_mprintf("%w", t.c_str());
            if (!escaped) continue;
            std::string countSql = "SELECT COUNT(*) FROM " + std::string(escaped);
            sqlite3_free(escaped);
            sqlite3_stmt* cs = nullptr;
            if (sqlite3_prepare_v2(db, countSql.c_str(), -1, &cs, nullptr) == SQLITE_OK) {
                if (sqlite3_step(cs) == SQLITE_ROW)
                    info.rowCount += sqlite3_column_int(cs, 0);
                sqlite3_finalize(cs);
            }
        }
    }

    sqlite3_close(db);
    return info;
}

// ==================== MANUAL UPDATE ====================

bool DBResolver::manualUpdate(const std::string& dbPath) {
    // Backup current database before attempting download
    std::string bakPath = dbPath + ".bak";
    if (std::filesystem::exists(dbPath)) {
        std::error_code ec;
        std::filesystem::remove(bakPath, ec);
        std::filesystem::copy_file(dbPath, bakPath, ec);
        if (!ec) {
            DBCacheManager::instance().setBackupHash(
                SHA256::hashFile(dbPath));
            DBCacheManager::instance().save();
        }
    }

    // Force fresh manifest fetch
    manifestFetched_ = false;

    Manifest m = fetchManifest();
    if (m.db_url.empty()) {
        if (tryRestoreFromBackup(bakPath, dbPath)) {
            DBCacheManager::instance().clearBackup();
            DBCacheManager::instance().save();
        }
        return false;
    }

    // Download to .tmp file for safe staging
    std::string tmpPath = dbPath + ".tmp";
    std::error_code ec;
    std::filesystem::remove(tmpPath, ec);

    if (!downloadDB(m.db_url, tmpPath)) {
        // Download failed — attempt restore from backup
        if (tryRestoreFromBackup(bakPath, dbPath)) {
            DBCacheManager::instance().clearBackup();
            DBCacheManager::instance().save();
        }
        return false;
    }

    // Validate the downloaded .tmp file
    sqlite3* db = nullptr;
    bool dlOk = false;
    if (sqlite3_open(tmpPath.c_str(), &db) == SQLITE_OK) {
        dlOk = validateSchema(db);
        sqlite3_close(db);
    }

    if (!dlOk) {
        std::filesystem::remove(tmpPath, ec);
        if (tryRestoreFromBackup(bakPath, dbPath)) {
            DBCacheManager::instance().clearBackup();
            DBCacheManager::instance().save();
        }
        return false;
    }

    std::string dlHash = SHA256::hashFile(tmpPath);
    if (dlHash != m.db_hash) {
        std::filesystem::remove(tmpPath, ec);
        if (tryRestoreFromBackup(bakPath, dbPath)) {
            DBCacheManager::instance().clearBackup();
            DBCacheManager::instance().save();
        }
        return false;
    }

    // All checks passed — atomically swap .tmp into place
    std::filesystem::rename(tmpPath, dbPath, ec);
    if (ec) {
        // Rename failed — fall back to copy
        ec.clear();
        std::filesystem::copy_file(tmpPath, dbPath,
            std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) {
            // copy failed — attempt cleanup + restore
            std::error_code ignore;
            std::filesystem::remove(tmpPath, ignore);
            if (tryRestoreFromBackup(bakPath, dbPath)) {
                DBCacheManager::instance().clearBackup();
                DBCacheManager::instance().save();
            }
            return false;
        }
        // copy succeeded — clean up .tmp (best-effort)
        std::error_code ignore;
        std::filesystem::remove(tmpPath, ignore);
    }

    // Success — update cache and clear backup hash
    dlHash = SHA256::hashFile(dbPath);
    DBCacheManager::instance().setLastSeenVersion(m.version);
    DBCacheManager::instance().setCurrentHash(dlHash);
    DBCacheManager::instance().recordAccess(dbPath, dlHash);
    DBCacheManager::instance().clearBackup();
    DBCacheManager::instance().save();

    // Invalidate all cached entries pointing to the old database file
    for (auto it = resolvedCache_.begin(); it != resolvedCache_.end(); ) {
        if (it->second == dbPath)
            it = resolvedCache_.erase(it);
        else
            ++it;
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
    std::string installPath = PathResolver::installDbFile().string();
    if (!std::filesystem::exists(installPath) ||
        !isSQLiteFile(installPath))
        return {};

    sqlite3* db = nullptr;
    bool defaultOk = false;
    if (sqlite3_open(installPath.c_str(), &db) == SQLITE_OK) {
        defaultOk = validateSchema(db);
        sqlite3_close(db);
    }
    if (!defaultOk) return {};

    std::error_code ec;
    // Always copy (never rename) — the install path is system-wide
    // and must not be moved or deleted.
    std::filesystem::copy_file(installPath, configPath,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        if (g_errorHandler.error) g_errorHandler.error(
            "Failed to copy default database to configured path.");
        return {};
    }
    std::string movedHash = SHA256::hashFile(configPath);
    return cacheResult(configPath, configPath, movedHash);
}

std::string DBResolver::resolveHashMismatch(const std::string& configPath,
                                             const std::string& hash) {
    std::string installPath = PathResolver::installDbFile().string();
    if (!std::filesystem::exists(installPath) ||
        !isSQLiteFile(installPath))
        return {};

    sqlite3* db = nullptr;
    bool defaultOk = false;
    if (sqlite3_open(installPath.c_str(), &db) == SQLITE_OK) {
        defaultOk = validateSchema(db);
        sqlite3_close(db);
    }
    if (!defaultOk) return {};

    std::error_code ec;
    std::filesystem::copy_file(installPath, configPath,
        std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        if (g_errorHandler.error) g_errorHandler.error(
            "Failed to copy default database.");
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

    // Step 1 — existing file is valid → use it
    if (fileExists && isSQLite && schemaOk)
        return cacheResult(configPath, configPath, hash);

    // Step 2 — try default DB
    {
        std::string result = copyDefaultToConfig(configPath);
        if (!result.empty()) return result;
    }

    // Step 3 — try download via manifest
    {
        // Backup current database before attempting download
        std::string bakPath = configPath + ".bak";
        if (std::filesystem::exists(configPath)) {
            std::error_code ec;
            std::filesystem::remove(bakPath, ec);
            std::filesystem::copy_file(configPath, bakPath, ec);
            if (!ec) {
                DBCacheManager::instance().setBackupHash(
                    SHA256::hashFile(configPath));
                DBCacheManager::instance().save();
            }
        }

        std::cout << "  database not found \u2014 fetching version manifest...\n"
                  << std::flush;

        Manifest m = fetchManifest();

        if (m.db_url.empty()) {
            if (g_errorHandler.error) g_errorHandler.error(
                "Could not fetch database manifest. "
                "The local database may be out of date. "
                "Check your internet connection to get the latest version.");
            return "";
        }

        std::cout << "  attempting download from GitHub...\n"
                  << std::flush;

        // Download to .tmp file for safe staging
        std::string tmpPath = configPath + ".tmp";
        std::error_code ec;

        // Clean up any stale .tmp from a previous interrupted download
        std::filesystem::remove(tmpPath, ec);

        if (!downloadDB(m.db_url, tmpPath)) {
            // Attempt to restore from backup before giving up
            if (tryRestoreFromBackup(bakPath, configPath)) {
                DBCacheManager::instance().clearBackup();
                DBCacheManager::instance().save();
                std::string restoredHash = SHA256::hashFile(configPath);
                return cacheResult(configPath, configPath, restoredHash);
            }
            if (g_errorHandler.error) g_errorHandler.error(
                "Failed to download database from GitHub. "
                "Check your internet connection and try again.");
            return "";
        }

        // Validate the downloaded .tmp file
        sqlite3* db   = nullptr;
        bool     dlOk = false;
        if (sqlite3_open(tmpPath.c_str(), &db) == SQLITE_OK) {
            dlOk = validateSchema(db);
            sqlite3_close(db);
        }

        if (!dlOk) {
            std::filesystem::remove(tmpPath, ec);
            // Attempt to restore from backup before giving up
            if (tryRestoreFromBackup(bakPath, configPath)) {
                DBCacheManager::instance().clearBackup();
                DBCacheManager::instance().save();
                std::string restoredHash = SHA256::hashFile(configPath);
                return cacheResult(configPath, configPath, restoredHash);
            }
            if (g_errorHandler.error) g_errorHandler.error(
                "Downloaded database failed schema validation.");
            return "";
        }

        std::string dlHash = SHA256::hashFile(tmpPath);
        if (dlHash != m.db_hash) {
            std::filesystem::remove(tmpPath, ec);
            // Attempt to restore from backup before giving up
            if (tryRestoreFromBackup(bakPath, configPath)) {
                DBCacheManager::instance().clearBackup();
                DBCacheManager::instance().save();
                std::string restoredHash = SHA256::hashFile(configPath);
                return cacheResult(configPath, configPath, restoredHash);
            }
            if (g_errorHandler.error) g_errorHandler.error(
                "Downloaded database hash does not match the version manifest. "
                "The file may have been tampered with. Download ignored.");
            return "";
        }

        // All checks passed — atomically swap .tmp into place
        std::filesystem::rename(tmpPath, configPath, ec);
        if (ec) {
            // Rename failed (rare — cross-device edge case) — fall back to copy
            ec.clear();
            std::filesystem::copy_file(tmpPath, configPath,
                std::filesystem::copy_options::overwrite_existing, ec);
            if (ec) {
                // copy failed — attempt cleanup + restore
                std::error_code ignore;
                std::filesystem::remove(tmpPath, ignore);
                if (tryRestoreFromBackup(bakPath, configPath)) {
                    DBCacheManager::instance().clearBackup();
                    DBCacheManager::instance().save();
                    std::string restoredHash = SHA256::hashFile(configPath);
                    return cacheResult(configPath, configPath, restoredHash);
                }
                return "";
            }
            // copy succeeded — clean up .tmp (best-effort)
            std::error_code ignore;
            std::filesystem::remove(tmpPath, ignore);
        }

        dlHash = SHA256::hashFile(configPath);
        return cacheResult(configPath, configPath, dlHash);
    }

    return "";
}
