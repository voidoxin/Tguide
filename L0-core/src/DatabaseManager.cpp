#include "DatabaseManager.h"
#include "ErrorHandler.h"
#include "db_cache_manager.h"
#include "DBResolver.h"
#include "sha256.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <functional>
#include <set>

bool DBFatal() { return DBResolver::instance().fatal(); }

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
            if (g_errorHandler.error) g_errorHandler.error("Failed to open database: " + db_path);
            db = nullptr;
        } else {
            sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, nullptr);
            sqlite3_exec(db, "PRAGMA journal_mode = WAL;", 0, 0, nullptr);
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
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            if (g_errorHandler.error) g_errorHandler.error(std::string("DB prepare failed: ") + sqlite3_errmsg(db));
            return false;
        }
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
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            if (g_errorHandler.error) g_errorHandler.error(std::string("DB prepare failed: ") + sqlite3_errmsg(db));
            return false;
        }
        if (binder) binder(stmt);
        int rc;
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            if (reader) reader(stmt);
        }
        if (rc == SQLITE_ERROR || rc == SQLITE_CORRUPT) {
            if (g_errorHandler.error) g_errorHandler.error(std::string("DB query error: ") + sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return false;
        }
        sqlite3_finalize(stmt);
        return true;
    }
};

// ==================== COLUMN WHITELISTS ====================

// column whitelists for getWhere() — prevents SQL injection via column names
static const std::set<std::string> SAFE_VULN_COLS = {
    "id","name","metasploit_name","discovered_date","discoverer",
    "severity","access","platform","service","description","danger"
};
static const std::set<std::string> SAFE_MODULE_COLS = {
    "id","name","path","platform","type","description","API","mode","loud","output"
};
static const std::set<std::string> SAFE_TOOL_COLS = {
    "id","name","category","short_desc","description","flags_all"
};
static const std::set<std::string> SAFE_CATEGORY_COLS = {
    "id","name","display_order","description"
};

// ==================== BackupManager ====================

#ifdef TGUIDE_DEV_MODE
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
        if (g_errorHandler.error) g_errorHandler.error(std::string("Backup failed: ") + e.what());
    }
    catch (...) {
        if (g_errorHandler.error) g_errorHandler.error("Backup failed: unknown error.");
    }
}
#endif

// ==================== VulnD ====================

VulnD::VulnD(const std::string& path) : db_path(DBResolver::instance().resolve(path)) {}
VulnD::VulnD(const std::string& path, DirectOpen) : db_path(path) {}

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
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

bool VulnD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM vulnerabilities WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
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
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

bool VulnD::delOption(int option_id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM options WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, option_id); }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
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

    for (auto& col : columns) {
        if (SAFE_VULN_COLS.find(col) == SAFE_VULN_COLS.end()) return results;
    }
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

ModuD::ModuD(const std::string& path) : db_path(DBResolver::instance().resolve(path)) {}
ModuD::ModuD(const std::string& path, DirectOpen) : db_path(path) {}

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
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

bool ModuD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM modules WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
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

    for (auto& col : columns) {
        if (SAFE_MODULE_COLS.find(col) == SAFE_MODULE_COLS.end()) return results;
    }
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

ToolD::ToolD(const std::string& path) : db_path(DBResolver::instance().resolve(path)) {}
ToolD::ToolD(const std::string& path, DirectOpen) : db_path(path) {}

bool ToolD::createTables() {
    DBSession s(db_path);
    if (!s.ok()) return false;

    return s.execute(
        "CREATE TABLE IF NOT EXISTS tools ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, category TEXT, short_desc TEXT, description TEXT, flags_all TEXT);"
    );
}

bool ToolD::add(const Tool& t) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "INSERT INTO tools(name, category, short_desc, description, flags_all)"
        " VALUES(?,?,?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, t.name.c_str(),        -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 2, t.category.c_str(),    -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 3, t.short_desc.c_str(),  -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 4, t.description.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_text(stmt, 5, t.flags_all.c_str(),   -1, SQLITE_TRANSIENT);
        }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

bool ToolD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM tools WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

ToolResults ToolD::getAll() {
    ToolResults results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    s.query(
        "SELECT id, name, category, short_desc, description, flags_all FROM tools",
        nullptr,
        [&](sqlite3_stmt* stmt) {
            Tool t;
            t.id          = sqlite3_column_int(stmt, 0);
            t.name        = col_text(stmt, 1);
            t.category    = col_text(stmt, 2);
            t.short_desc  = col_text(stmt, 3);
            t.description = col_text(stmt, 4);
            t.flags_all   = col_text(stmt, 5);
            results.items.push_back(t);
        }
    );
    return results;
}

ToolResults ToolD::getWhere(const std::vector<std::string>& columns,
                             const std::vector<std::string>& values) {
    ToolResults results;
    if (columns.size() != values.size() || columns.empty()) return results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    for (auto& col : columns) {
        if (SAFE_TOOL_COLS.find(col) == SAFE_TOOL_COLS.end()) return results;
    }
    std::stringstream ss;
    ss << "SELECT id, name, category, short_desc, description, flags_all FROM tools WHERE ";
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
            Tool t;
            t.id          = sqlite3_column_int(stmt, 0);
            t.name        = col_text(stmt, 1);
            t.category    = col_text(stmt, 2);
            t.short_desc  = col_text(stmt, 3);
            t.description = col_text(stmt, 4);
            t.flags_all   = col_text(stmt, 5);
            results.items.push_back(t);
        }
    );
    return results;
}

// ==================== ToolFlagD ====================

ToolFlagD::ToolFlagD(const std::string& path) : db_path(DBResolver::instance().resolve(path)) {}
ToolFlagD::ToolFlagD(const std::string& path, DirectOpen) : db_path(path) {}

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
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

bool ToolFlagD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM tool_flags WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
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

TemplateD::TemplateD(const std::string& path) : db_path(DBResolver::instance().resolve(path)) {}
TemplateD::TemplateD(const std::string& path, DirectOpen) : db_path(path) {}

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
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

bool TemplateD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM templates WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
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

// ==================== CategoryD ====================

CategoryD::CategoryD(const std::string& path) : db_path(DBResolver::instance().resolve(path)) {}
CategoryD::CategoryD(const std::string& path, DirectOpen) : db_path(path) {}

bool CategoryD::createTables() {
    DBSession s(db_path);
    if (!s.ok()) return false;

    return s.execute(
        "CREATE TABLE IF NOT EXISTS categories ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, display_order INTEGER, description TEXT);"
    );
}

bool CategoryD::add(const Category& c) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "INSERT INTO categories(name, display_order, description)"
        " VALUES(?,?,?)",
        [&](sqlite3_stmt* stmt) {
            sqlite3_bind_text(stmt, 1, c.name.c_str(),         -1, SQLITE_TRANSIENT);
            sqlite3_bind_int (stmt, 2, c.display_order);
            sqlite3_bind_text(stmt, 3, c.description.c_str(),  -1, SQLITE_TRANSIENT);
        }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

bool CategoryD::del(int id) {
    DBSession s(db_path);
    if (!s.ok()) return false;

    bool ok = s.execute(
        "DELETE FROM categories WHERE id=?",
        [&](sqlite3_stmt* stmt) { sqlite3_bind_int(stmt, 1, id); }
    );
#ifdef TGUIDE_DEV_MODE
    if (ok) BackupManager::backupDatabase(db_path);
#endif
    return ok;
}

CategoryResults CategoryD::getAll() {
    CategoryResults results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    s.query(
        "SELECT id, name, display_order, description FROM categories",
        nullptr,
        [&](sqlite3_stmt* stmt) {
            Category c;
            c.id            = sqlite3_column_int(stmt, 0);
            c.name          = col_text(stmt, 1);
            c.display_order = sqlite3_column_int(stmt, 2);
            c.description   = col_text(stmt, 3);
            results.items.push_back(c);
        }
    );
    return results;
}

CategoryResults CategoryD::getWhere(const std::vector<std::string>& columns,
                                     const std::vector<std::string>& values) {
    CategoryResults results;
    if (columns.size() != values.size() || columns.empty()) return results;
    DBSession s(db_path);
    if (!s.ok()) return results;

    for (auto& col : columns) {
        if (SAFE_CATEGORY_COLS.find(col) == SAFE_CATEGORY_COLS.end()) return results;
    }
    std::stringstream ss;
    ss << "SELECT id, name, display_order, description FROM categories WHERE ";
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
            Category c;
            c.id            = sqlite3_column_int(stmt, 0);
            c.name          = col_text(stmt, 1);
            c.display_order = sqlite3_column_int(stmt, 2);
            c.description   = col_text(stmt, 3);
            results.items.push_back(c);
        }
    );
    return results;
}
