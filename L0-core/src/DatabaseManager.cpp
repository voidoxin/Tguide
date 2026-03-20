#include "DatabaseManager.h"
#include "db_cache_manager.h"
#include "sha256.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <set>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <curl/curl.h>

extern char UI_attention(const std::string& msg);
extern void UI_errors(const std::string& msg);
extern void UI_fatal(const std::string& msg);

static bool s_fatal          = false;
static bool s_cacheValidated = false;

bool DBFatal() { return s_fatal; }

static std::string col_text(sqlite3_stmt* stmt, int col) {
    const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    return t ? t : "";
}

// ==================== DBSession ====================

class DBSession {
    sqlite3*    db;
    std::string db_path;
public:
    DBSession(const std::string& path) : db(nullptr), db_path(path) {
        if (path.empty()) return;
        if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
            UI_errors("Failed to open database: " + db_path);
            db = nullptr;
        } else {
            sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, nullptr);
        }
    }

    ~DBSession() {
        if (db) sqlite3_close(db);
    }

    DBSession(const DBSession&)            = delete;
    DBSession& operator=(const DBSession&) = delete;

    bool ok() const { return db != nullptr; }

    bool execute(const char* sql,
                 std::function<void(sqlite3_stmt*)> binder = nullptr) {
        if (!db) return false;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        if (binder) binder(stmt);
        bool result = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return result;
    }

    bool query(const char* sql,
               std::function<void(sqlite3_stmt*)> binder,
               std::function<void(sqlite3_stmt*)> reader) {
        if (!db) return false;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        if (binder) binder(stmt);
        while (sqlite3_step(stmt) == SQLITE_ROW)
            if (reader) reader(stmt);
        sqlite3_finalize(stmt);
        return true;
    }
};

// ==================== VALIDATION ====================

static const std::string DEFAULT_DB_PATH = "data/database/tguide.db";

static bool isSafePath(const std::string& s) {
    for (char c : s) {
        if (c == ';' || c == '&' || c == '|' ||
            c == '`' || c == '$' || c == '\n' || c == '\r')
            return false;
    }
    return true;
}

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
        { "tools",           { "id","name","description","flags_all" } },
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

static bool downloadDB(const std::string& url, const std::string& destPath) {
    if (url.empty())           return false;
    if (!isSafePath(url))      return false;
    if (!isSafePath(destPath)) return false;

    FILE* f = fopen(destPath.c_str(), "wb");
    if (!f) return false;

    CURL* curl = curl_easy_init();
    if (!curl) {
        fclose(f);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      f);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR,    1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(f);

    if (res != CURLE_OK) {
        std::error_code ec;
        std::filesystem::remove(destPath, ec);
        return false;
    }

    // Remove file if downloaded content is not a valid SQLite database
    if (!isSQLiteFile(destPath)) {
        std::error_code ec;
        std::filesystem::remove(destPath, ec);
        return false;
    }

    return true;
}

// ==================== RESOLVER ====================

static std::unordered_map<std::string, std::string> s_resolvedCache;

static void invalidateCacheIfMissing() {
    if (s_cacheValidated) return;
    for (auto it = s_resolvedCache.begin(); it != s_resolvedCache.end(); ) {
        if (!std::filesystem::exists(it->second))
            it = s_resolvedCache.erase(it);
        else
            ++it;
    }
    s_cacheValidated = true;
}

static std::string resolveDatabase(const std::string& configPath) {
    if (s_fatal) return "";

    invalidateCacheIfMissing();

    auto it = s_resolvedCache.find(configPath);
    if (it != s_resolvedCache.end()) return it->second;

    std::filesystem::create_directories(
        std::filesystem::path(configPath).parent_path()
    );

    bool        fileExists = std::filesystem::exists(configPath);
    bool        isSQLite   = fileExists && isSQLiteFile(configPath);
    bool        schemaOk   = false;
    bool        hasData    = false;
    std::string hash;

    if (isSQLite) {
        sqlite3* db = nullptr;
        if (sqlite3_open(configPath.c_str(), &db) == SQLITE_OK) {
            schemaOk = validateSchema(db);
            hasData  = schemaOk && dbHasData(db);
            sqlite3_close(db);
        }
        hash = SHA256::hashFile(configPath);
    }

    const bool officialHashSet = !std::string(DB_OFFICIAL_HASH).empty();
    const bool isOfficial      = officialHashSet && hash == DB_OFFICIAL_HASH;

    // Case 1 — valid, schema ok, hash matches official (or not set yet)
    if (fileExists && isSQLite && schemaOk && (isOfficial || !officialHashSet)) {
        DBCache::setCurrentHash(hash);
        DBCache::recordAccess(configPath, hash);
        DBCache::save(configPath);
        s_resolvedCache[configPath] = configPath;
        return configPath;
    }

    // Case 2 — valid SQLite, correct schema, has data, hash differs from official
    if (fileExists && isSQLite && schemaOk && hasData && officialHashSet && !isOfficial) {
        char response = UI_attention(
            "The database at the configured path does not match the official release. "
            "Enter 'y' to revert to the official database, or 'n' to keep the external one."
        );

        bool useExternal = (response == 'n' || response == 'N');

        if (useExternal) {
            DBCache::setCurrentHash(hash);
            DBCache::recordAccess(configPath, hash);
            DBCache::save(configPath);
            s_resolvedCache[configPath] = configPath;
            return configPath;
        }

        if (std::filesystem::exists(DEFAULT_DB_PATH) &&
            isSQLiteFile(DEFAULT_DB_PATH)) {
            std::string defaultHash     = SHA256::hashFile(DEFAULT_DB_PATH);
            bool        defaultOfficial = officialHashSet &&
                                          defaultHash == DB_OFFICIAL_HASH;
            if (defaultOfficial) {
                std::error_code ec;
                std::filesystem::copy_file(DEFAULT_DB_PATH, configPath,
                    std::filesystem::copy_options::overwrite_existing, ec);
                if (!ec) {
                    DBCache::setCurrentHash(defaultHash);
                    DBCache::recordAccess(configPath, defaultHash);
                    DBCache::save(configPath);
                    s_resolvedCache[configPath] = configPath;
                    return configPath;
                }
                UI_errors("Failed to copy official database. Attempting recovery.");
            }
        }
        fileExists = false;
    }

    // Case 3a — locate valid DB at default path and move it to config path
    if (std::filesystem::exists(DEFAULT_DB_PATH) &&
        isSQLiteFile(DEFAULT_DB_PATH)) {
        sqlite3* db        = nullptr;
        bool     defaultOk = false;

        if (sqlite3_open(DEFAULT_DB_PATH.c_str(), &db) == SQLITE_OK) {
            defaultOk = validateSchema(db);
            sqlite3_close(db);
        }

        if (defaultOk) {
            std::error_code ec;
            std::filesystem::rename(DEFAULT_DB_PATH, configPath, ec);
            if (ec) {
                std::filesystem::copy_file(DEFAULT_DB_PATH, configPath,
                    std::filesystem::copy_options::overwrite_existing, ec);
                if (!ec) std::filesystem::remove(DEFAULT_DB_PATH);
            }

            if (!ec) {
                std::string movedHash = SHA256::hashFile(configPath);
                DBCache::setCurrentHash(movedHash);
                DBCache::recordAccess(configPath, movedHash);
                DBCache::save(configPath);
                s_resolvedCache[configPath] = configPath;
                return configPath;
            }
            UI_errors("Failed to move default database to configured path.");
        }
    }

    // Case 3b — download from GitHub via libcurl
    std::string downloadUrl = std::string(DB_DOWNLOAD_URL);

    if (!downloadUrl.empty()) {
        UI_errors("Database not found locally. Attempting download from GitHub.");

        if (downloadDB(downloadUrl, configPath)) {
            sqlite3* db   = nullptr;
            bool     dlOk = false;

            if (sqlite3_open(configPath.c_str(), &db) == SQLITE_OK) {
                dlOk = validateSchema(db);
                sqlite3_close(db);
            }

            if (dlOk) {
                std::string dlHash = SHA256::hashFile(configPath);
                DBCache::setCurrentHash(dlHash);
                DBCache::recordAccess(configPath, dlHash);
                DBCache::save(configPath);
                s_resolvedCache[configPath] = configPath;
                return configPath;
            }

            std::error_code ec;
            std::filesystem::remove(configPath, ec);
        }

        UI_fatal(
            "Failed to download or validate the official database. "
            "Please check your internet connection or verify "
            "DB_DOWNLOAD_URL in db_cache_manager.h."
        );
    } else {
        UI_fatal(
            "Database not found and DB_DOWNLOAD_URL is not configured. "
            "Set DB_DOWNLOAD_URL in db_cache_manager.h before release."
        );
    }

    s_fatal = true;
    return "";
}

// ==================== BackupManager ====================

std::string BackupManager::s_backupPath;

void BackupManager::init(const std::string& backupPath) {
    s_backupPath = backupPath;
}

void BackupManager::backupDatabase(const std::string& originalPath) {
    if (s_backupPath.empty() || originalPath.empty()) return;
    try {
        std::filesystem::create_directories(
            std::filesystem::path(s_backupPath).parent_path()
        );
        std::filesystem::copy_file(originalPath, s_backupPath,
            std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::exception& e) {
        UI_errors(std::string("Backup failed: ") + e.what());
    }
    catch (...) {
        UI_errors("Backup failed: unknown error.");
    }
}

// ==================== VulnD ====================

VulnD::VulnD(const std::string& path) : db_path(resolveDatabase(path)) {}

bool VulnD::createTables() {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "CREATE TABLE IF NOT EXISTS vulnerabilities ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, metasploit_name TEXT, discovered_date TEXT, discoverer TEXT,"
        "severity TEXT, access TEXT, platform TEXT, service TEXT,"
        "description TEXT, danger TEXT);"
    );
    ok &= s.execute(
        "CREATE TABLE IF NOT EXISTS options ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, vuln_id INTEGER,"
        "option_name TEXT, option_value TEXT,"
        "FOREIGN KEY(vuln_id) REFERENCES vulnerabilities(id) ON DELETE CASCADE);"
    );
    return ok;
}

bool VulnD::add(const Vulnerability& v) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "INSERT INTO vulnerabilities"
        "(name, metasploit_name, discovered_date, discoverer,"
        " severity, access, platform, service, description, danger)"
        " VALUES(?,?,?,?,?,?,?,?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1,  v.name.c_str(),            -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2,  v.metasploit.c_str(),      -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3,  v.discovered_date.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4,  v.discoverer.c_str(),      -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5,  v.severity.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6,  v.access.c_str(),          -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 7,  v.platform.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 8,  v.service.c_str(),         -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 9,  v.description.c_str(),     -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 10, v.danger.c_str(),          -1, SQLITE_TRANSIENT);
        }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

bool VulnD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM vulnerabilities WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

bool VulnD::addOption(const Option& o) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "INSERT INTO options(vuln_id, option_name, option_value) VALUES(?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_int (stmt, 1, o.vuln_id);
            sqlite3_bind_text(stmt, 2, o.name.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, o.value.c_str(), -1, SQLITE_TRANSIENT);
        }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

bool VulnD::delOption(int option_id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM options WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, option_id); }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

std::vector<Vulnerability> VulnD::getAll() {
    std::vector<Vulnerability> list;
    DBSession s(db_path);
    if (!s.ok()) return list;

    s.query(
        "SELECT id, name, metasploit_name, discovered_date, discoverer,"
        " severity, access, platform, service, description, danger"
        " FROM vulnerabilities",
        nullptr,
        [&](sqlite3_stmt* stmt) {
            Vulnerability v;
            v.id              = sqlite3_column_int(stmt, 0);
            v.name            = col_text(stmt, 1);
            v.metasploit      = col_text(stmt, 2);
            v.discovered_date = col_text(stmt, 3);
            v.discoverer      = col_text(stmt, 4);
            v.severity        = col_text(stmt, 5);
            v.access          = col_text(stmt, 6);
            v.platform        = col_text(stmt, 7);
            v.service         = col_text(stmt, 8);
            v.description     = col_text(stmt, 9);
            v.danger          = col_text(stmt, 10);
            list.push_back(v);
        }
    );
    return list;
}

std::vector<Option> VulnD::getOptions(int vuln_id) {
    std::vector<Option> list;
    DBSession s(db_path);
    if (!s.ok()) return list;

    s.query(
        "SELECT id, vuln_id, option_name, option_value FROM options WHERE vuln_id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, vuln_id); },
        [&](sqlite3_stmt* stmt) {
            Option o;
            o.id      = sqlite3_column_int(stmt, 0);
            o.vuln_id = sqlite3_column_int(stmt, 1);
            o.name    = col_text(stmt, 2);
            o.value   = col_text(stmt, 3);
            list.push_back(o);
        }
    );
    return list;
}

VulnResults VulnD::getWhere(const std::vector<std::string>& columns,
                             const std::vector<std::string>& values) {
    VulnResults results;
    if (columns.size() != values.size() || columns.empty()) return results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    std::stringstream ss;
    ss << "SELECT id, name, metasploit_name, discovered_date, discoverer,"
          " severity, access, platform, service, description, danger"
          " FROM vulnerabilities WHERE ";
    for (size_t i = 0; i < columns.size(); i++) {
        ss << columns[i] << "=?";
        if (i < columns.size() - 1) ss << " AND ";
    }
    std::string sql = ss.str();

    s.query(
        sql.c_str(),
        [&](sqlite3_stmt* stmt) {
            for (size_t i = 0; i < values.size(); i++)
                sqlite3_bind_text(stmt, (int)i + 1,
                    values[i].c_str(), -1, SQLITE_TRANSIENT);
        },
        [&](sqlite3_stmt* stmt) {
            Vulnerability v;
            v.id              = sqlite3_column_int(stmt, 0);
            v.name            = col_text(stmt, 1);
            v.metasploit      = col_text(stmt, 2);
            v.discovered_date = col_text(stmt, 3);
            v.discoverer      = col_text(stmt, 4);
            v.severity        = col_text(stmt, 5);
            v.access          = col_text(stmt, 6);
            v.platform        = col_text(stmt, 7);
            v.service         = col_text(stmt, 8);
            v.description     = col_text(stmt, 9);
            v.danger          = col_text(stmt, 10);
            results.items.push_back(v);
        }
    );
    return results;
}

// ==================== ModuD ====================

ModuD::ModuD(const std::string& path) : db_path(resolveDatabase(path)) {}

bool ModuD::createTables() {
    DBSession s(db_path);
    if (!s.ok()) return false;

    return s.execute(
        "CREATE TABLE IF NOT EXISTS modules ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, path TEXT, platform TEXT, type TEXT, description TEXT,"
        "API TEXT, mode INTEGER, loud INTEGER, output TEXT);"
    );
}

bool ModuD::add(const Module& m) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    Module copy = m;
    if (!copy.mode) copy.loud = false;

    bool ok = s.execute(
        "INSERT INTO modules(name, path, platform, type, description, API, mode, loud, output)"
        " VALUES(?,?,?,?,?,?,?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, copy.name.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, copy.path.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, copy.platform.c_str(),    -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, copy.type.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, copy.description.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 6, copy.API.c_str(),         -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (stmt, 7, copy.mode ? 1 : 0);
            sqlite3_bind_int (stmt, 8, copy.loud ? 1 : 0);
            sqlite3_bind_text(stmt, 9, copy.output.c_str(),      -1, SQLITE_TRANSIENT);
        }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

bool ModuD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM modules WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

std::vector<Module> ModuD::getAll() {
    std::vector<Module> list;
    DBSession s(db_path);
    if (!s.ok()) return list;

    s.query(
        "SELECT id, name, path, platform, type, description, API, mode, loud, output"
        " FROM modules",
        nullptr,
        [&](sqlite3_stmt* stmt) {
            Module m;
            m.id          = sqlite3_column_int(stmt, 0);
            m.name        = col_text(stmt, 1);
            m.path        = col_text(stmt, 2);
            m.platform    = col_text(stmt, 3);
            m.type        = col_text(stmt, 4);
            m.description = col_text(stmt, 5);
            m.API         = col_text(stmt, 6);
            m.mode        = sqlite3_column_int(stmt, 7) != 0;
            m.loud        = sqlite3_column_int(stmt, 8) != 0;
            m.output      = col_text(stmt, 9);
            list.push_back(m);
        }
    );
    return list;
}

ModuleResults ModuD::getWhere(const std::vector<std::string>& columns,
                               const std::vector<std::string>& values) {
    ModuleResults results;
    if (columns.size() != values.size() || columns.empty()) return results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    std::stringstream ss;
    ss << "SELECT id, name, path, platform, type, description, API, mode, loud, output"
          " FROM modules WHERE ";
    for (size_t i = 0; i < columns.size(); i++) {
        ss << columns[i] << "=?";
        if (i < columns.size() - 1) ss << " AND ";
    }
    std::string sql = ss.str();

    s.query(
        sql.c_str(),
        [&](sqlite3_stmt* stmt) {
            for (size_t i = 0; i < values.size(); i++)
                sqlite3_bind_text(stmt, (int)i + 1,
                    values[i].c_str(), -1, SQLITE_TRANSIENT);
        },
        [&](sqlite3_stmt* stmt) {
            Module m;
            m.id          = sqlite3_column_int(stmt, 0);
            m.name        = col_text(stmt, 1);
            m.path        = col_text(stmt, 2);
            m.platform    = col_text(stmt, 3);
            m.type        = col_text(stmt, 4);
            m.description = col_text(stmt, 5);
            m.API         = col_text(stmt, 6);
            m.mode        = sqlite3_column_int(stmt, 7) != 0;
            m.loud        = sqlite3_column_int(stmt, 8) != 0;
            m.output      = col_text(stmt, 9);
            results.items.push_back(m);
        }
    );
    return results;
}

// ==================== ToolD ====================

ToolD::ToolD(const std::string& path) : db_path(resolveDatabase(path)) {}

bool ToolD::createTables() {
    DBSession s(db_path);
    if (!s.ok()) return false;

    return s.execute(
        "CREATE TABLE IF NOT EXISTS tools ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, description TEXT, flags_all TEXT);"
    );
}

bool ToolD::add(const Tool& t) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "INSERT INTO tools(name, description, flags_all) VALUES(?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, t.name.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, t.description.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, t.flags_all.c_str(),   -1, SQLITE_TRANSIENT);
        }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

bool ToolD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM tools WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

ToolResults ToolD::getAll() {
    ToolResults results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    s.query(
        "SELECT id, name, description, flags_all FROM tools",
        nullptr,
        [&](sqlite3_stmt* stmt) {
            Tool t;
            t.id          = sqlite3_column_int(stmt, 0);
            t.name        = col_text(stmt, 1);
            t.description = col_text(stmt, 2);
            t.flags_all   = col_text(stmt, 3);
            results.items.push_back(t);
        }
    );
    return results;
}

// ==================== ToolFlagD ====================

ToolFlagD::ToolFlagD(const std::string& path) : db_path(resolveDatabase(path)) {}

bool ToolFlagD::createTables() {
    DBSession s(db_path);
    if (!s.ok()) return false;

    return s.execute(
        "CREATE TABLE IF NOT EXISTS tool_flags ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "tool_id INTEGER, name TEXT, description TEXT,"
        "loud TEXT, root INTEGER, protocols TEXT,"
        "FOREIGN KEY(tool_id) REFERENCES tools(id) ON DELETE CASCADE);"
    );
}

bool ToolFlagD::add(const ToolFlag& f) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "INSERT INTO tool_flags(tool_id, name, description, loud, root, protocols)"
        " VALUES(?,?,?,?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_int (stmt, 1, f.tool_id);
            sqlite3_bind_text(stmt, 2, f.name.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, f.description.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, f.loud.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (stmt, 5, f.root ? 1 : 0);
            sqlite3_bind_text(stmt, 6, f.protocols.c_str(),   -1, SQLITE_TRANSIENT);
        }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

bool ToolFlagD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM tool_flags WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

ToolFlagResults ToolFlagD::getWhere(int tool_id) {
    ToolFlagResults results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    s.query(
        "SELECT id, tool_id, name, description, loud, root, protocols"
        " FROM tool_flags WHERE tool_id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, tool_id); },
        [&](sqlite3_stmt* stmt) {
            ToolFlag f;
            f.id          = sqlite3_column_int(stmt, 0);
            f.tool_id     = sqlite3_column_int(stmt, 1);
            f.name        = col_text(stmt, 2);
            f.description = col_text(stmt, 3);
            f.loud        = col_text(stmt, 4);
            f.root        = sqlite3_column_int(stmt, 5) != 0;
            f.protocols   = col_text(stmt, 6);
            results.items.push_back(f);
        }
    );
    return results;
}

// ==================== TemplateD ====================

TemplateD::TemplateD(const std::string& path) : db_path(resolveDatabase(path)) {}

bool TemplateD::createTables() {
    DBSession s(db_path);
    if (!s.ok()) return false;

    return s.execute(
        "CREATE TABLE IF NOT EXISTS templates ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "tool_id INTEGER, template_name TEXT, description TEXT,"
        "root INTEGER, protocols TEXT, flag INTEGER,"
        "FOREIGN KEY(tool_id) REFERENCES tools(id) ON DELETE CASCADE);"
    );
}

bool TemplateD::add(const Template& t) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "INSERT INTO templates(tool_id, template_name, description, root, protocols, flag)"
        " VALUES(?,?,?,?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_int (stmt, 1, t.tool_id);
            sqlite3_bind_text(stmt, 2, t.template_name.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, t.description.c_str(),   -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (stmt, 4, t.root ? 1 : 0);
            sqlite3_bind_text(stmt, 5, t.protocols.c_str(),     -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (stmt, 6, t.flag ? 1 : 0);
        }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

bool TemplateD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM templates WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
    if (ok) BackupManager::backupDatabase(db_path);
    return ok;
}

TemplateResults TemplateD::getWhere(int tool_id) {
    TemplateResults results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    s.query(
        "SELECT id, tool_id, template_name, description, root, protocols, flag"
        " FROM templates WHERE tool_id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, tool_id); },
        [&](sqlite3_stmt* stmt) {
            Template t;
            t.id            = sqlite3_column_int(stmt, 0);
            t.tool_id       = sqlite3_column_int(stmt, 1);
            t.template_name = col_text(stmt, 2);
            t.description   = col_text(stmt, 3);
            t.root          = sqlite3_column_int(stmt, 4) != 0;
            t.protocols     = col_text(stmt, 5);
            t.flag          = sqlite3_column_int(stmt, 6) != 0;
            results.items.push_back(t);
        }
    );
    return results;
}