#include "DatabaseManager.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include <functional>

// ==================== HELPER ====================

// NULL-safe column reader — sqlite3_column_text returns nullptr for NULL values
// assigning nullptr to std::string is undefined behavior / crash
static std::string col_text(sqlite3_stmt* stmt, int col) {
    const char* t = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    return t ? t : "";
}

// ==================== DBSession ====================
// Internal class — not visible outside this file
// RAII: opens connection on construction, closes on destruction automatically

class DBSession {
    sqlite3*    db;
    std::string db_path;
public:
    DBSession(const std::string& path) : db(nullptr), db_path(path) {
        if (sqlite3_open(db_path.c_str(), &db) != SQLITE_OK) {
            std::cerr << "DB OPEN ERROR: " << sqlite3_errmsg(db) << std::endl;
            db = nullptr;
        } else {
            sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, nullptr);
        }
    }

    ~DBSession() {
        if (db) sqlite3_close(db); // RAII — always closed, even if exception happens
    }

    // Non-copyable — one session, one connection
    DBSession(const DBSession&)            = delete;
    DBSession& operator=(const DBSession&) = delete;

    bool ok() const { return db != nullptr; }

    // For INSERT / DELETE / CREATE — binder is optional
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

    // For SELECT — binder binds WHERE params (optional), reader reads each row (null-safe)
    bool query(const char* sql,
               std::function<void(sqlite3_stmt*)> binder,
               std::function<void(sqlite3_stmt*)> reader) {
        if (!db) return false;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
        if (binder) binder(stmt);
        while (sqlite3_step(stmt) == SQLITE_ROW)
            if (reader) reader(stmt); // null-safe — prevents crash if reader not provided
        sqlite3_finalize(stmt);
        return true;
    }
};

// ==================== BackupManager ====================

std::string BackupManager::s_backupPath;

void BackupManager::init(const std::string& backupPath) {
    s_backupPath = backupPath;
}

void BackupManager::backupDatabase(const std::string& originalPath) {
    if (s_backupPath.empty()) return;
    try {
        // Create backup directory if it doesn't exist — prevents silent failure
        std::filesystem::create_directories(
            std::filesystem::path(s_backupPath).parent_path()
        );
        // overwrite_existing handles deletion internally — no need for remove() first
        std::filesystem::copy_file(originalPath, s_backupPath,
            std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::exception& e) {
        std::cerr << "Backup error: " << e.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Backup unknown error" << std::endl;
    }
}

// ==================== VulnD ====================

VulnD::VulnD(const std::string& path) : db_path(path) {}

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
                sqlite3_bind_text(stmt, (int)i + 1, values[i].c_str(), -1, SQLITE_TRANSIENT);
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

ModuD::ModuD(const std::string& path) : db_path(path) {}

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
    if (!copy.mode) copy.loud = false; // passive mode cannot be loud

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
                sqlite3_bind_text(stmt, (int)i + 1, values[i].c_str(), -1, SQLITE_TRANSIENT);
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

ToolD::ToolD(const std::string& path) : db_path(path) {}

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

ToolFlagD::ToolFlagD(const std::string& path) : db_path(path) {}

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

TemplateD::TemplateD(const std::string& path) : db_path(path) {}

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