#include "DatabaseManager.h"
#include <iostream>
#include <sstream>
#include <filesystem>
#include "../include/config_manager.h"

// ==================== VulnD ====================

VulnD::VulnD(const std::string& path) : db(nullptr), db_path(path) {
    if (sqlite3_open(db_path.c_str(), &db)) {
        std::cerr << "DB OPEN ERROR: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

VulnD::~VulnD() {
    if (db) sqlite3_close(db);
}

bool VulnD::createTables() {
    if (!db) return false;

    const char* sql_vuln =
        "CREATE TABLE IF NOT EXISTS vulnerabilities ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, metasploit_name TEXT, discovered_date TEXT, discoverer TEXT,"
        "severity TEXT, access TEXT, platform TEXT, service TEXT, description TEXT, danger TEXT"
        ");";

    const char* sql_options =
        "CREATE TABLE IF NOT EXISTS options ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, vuln_id INTEGER,"
        "option_name TEXT, option_value TEXT,"
        "FOREIGN KEY(vuln_id) REFERENCES vulnerabilities(id) ON DELETE CASCADE"
        ");";

    char* err;
    if (sqlite3_exec(db, sql_vuln, 0, 0, &err) != SQLITE_OK) {
        std::cerr << "VULN TABLE ERROR: " << err << std::endl;
        sqlite3_free(err);
        return false;
    }
    if (sqlite3_exec(db, sql_options, 0, 0, &err) != SQLITE_OK) {
        std::cerr << "OPTIONS TABLE ERROR: " << err << std::endl;
        sqlite3_free(err);
        return false;
    }
    return true;
}

// ----- Add / Delete -----
bool VulnD::add(const Vulnerability& v) {
    if (!db) return false;
    const char* sql =
        "INSERT INTO vulnerabilities (name, metasploit_name, discovered_date, discoverer,"
        "severity, access, platform, service, description, danger) VALUES (?,?,?,?,?,?,?,?,?,?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, v.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, v.metasploit.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, v.discovered_date.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, v.discoverer.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, v.severity.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, v.access.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, v.platform.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, v.service.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 9, v.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 10, v.danger.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt); BackupManager::backupDatabase(db_path);
    return ok;
}

bool VulnD::del(int id) {
    if (!db) return false;
    const char* sql = "DELETE FROM vulnerabilities WHERE id=?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
BackupManager::backupDatabase(db_path);
    return ok;
}

// ----- Options -----
bool VulnD::addOption(const Option& o) {
    if (!db) return false;
    const char* sql = "INSERT INTO options(vuln_id, option_name, option_value) VALUES(?,?,?)";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, o.vuln_id);
    sqlite3_bind_text(stmt, 2, o.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, o.value.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
BackupManager::backupDatabase(db_path);
    return ok;
}

bool VulnD::delOption(int option_id) {
    if (!db) return false;
    const char* sql = "DELETE FROM options WHERE id=?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, option_id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
BackupManager::backupDatabase(db_path);
    return ok;
}

std::vector<Vulnerability> VulnD::getAll() {
    std::vector<Vulnerability> list;
    if (!db) return list;

    const char* sql = "SELECT * FROM vulnerabilities";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Vulnerability v;
        v.id = sqlite3_column_int(stmt, 0);
        v.name = (char*)sqlite3_column_text(stmt, 1);
        v.metasploit = (char*)sqlite3_column_text(stmt, 2);
        v.discovered_date = (char*)sqlite3_column_text(stmt, 3);
        v.discoverer = (char*)sqlite3_column_text(stmt, 4);
        v.severity = (char*)sqlite3_column_text(stmt, 5);
        v.access = (char*)sqlite3_column_text(stmt, 6);
        v.platform = (char*)sqlite3_column_text(stmt, 7);
        v.service = (char*)sqlite3_column_text(stmt, 8);
        v.description = (char*)sqlite3_column_text(stmt, 9);
        v.danger = (char*)sqlite3_column_text(stmt, 10);
        list.push_back(v);
    }

    sqlite3_finalize(stmt);
    return list;
}

std::vector<Option> VulnD::getOptions(int vuln_id) {
    std::vector<Option> list;
    if (!db) return list;

    const char* sql = "SELECT * FROM options WHERE vuln_id=?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;
    sqlite3_bind_int(stmt, 1, vuln_id);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Option o;
        o.id = sqlite3_column_int(stmt, 0);
        o.vuln_id = sqlite3_column_int(stmt, 1);
        o.name = (char*)sqlite3_column_text(stmt, 2);
        o.value = (char*)sqlite3_column_text(stmt, 3);
        list.push_back(o);
    }

    sqlite3_finalize(stmt);
    return list;
}

VulnResults VulnD::getWhere(const std::vector<std::string>& columns,
                             const std::vector<std::string>& values) {
    VulnResults results;
    if (!db) return results;
    if (columns.size() != values.size() || columns.empty()) return results;

    std::stringstream ss;
    ss << "SELECT * FROM vulnerabilities WHERE ";
    for (size_t i = 0; i < columns.size(); i++) {
        ss << columns[i] << "=?";
        if (i < columns.size() - 1) ss << " AND ";
    }

    std::string sql = ss.str();
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    for (size_t i = 0; i < values.size(); i++)
        sqlite3_bind_text(stmt, i + 1, values[i].c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Vulnerability v;
        v.id = sqlite3_column_int(stmt, 0);
        v.name = (char*)sqlite3_column_text(stmt, 1);
        v.metasploit = (char*)sqlite3_column_text(stmt, 2);
        v.discovered_date = (char*)sqlite3_column_text(stmt, 3);
        v.discoverer = (char*)sqlite3_column_text(stmt, 4);
        v.severity = (char*)sqlite3_column_text(stmt, 5);
        v.access = (char*)sqlite3_column_text(stmt, 6);
        v.platform = (char*)sqlite3_column_text(stmt, 7);
        v.service = (char*)sqlite3_column_text(stmt, 8);
        v.description = (char*)sqlite3_column_text(stmt, 9);
        v.danger = (char*)sqlite3_column_text(stmt, 10);
        results.items.push_back(v);
    }

    sqlite3_finalize(stmt);
    return results;
}

// ==================== ModuD ====================

ModuD::ModuD(const std::string& path) : db(nullptr), db_path(path) {
    if (sqlite3_open(db_path.c_str(), &db)) {
        std::cerr << "MODULE DB OPEN ERROR: " << sqlite3_errmsg(db) << std::endl;
        db = nullptr;
    }
}

ModuD::~ModuD() {
         if (db) sqlite3_close(db);
         }

bool ModuD::createTables() {
    if (!db) return false;

    const char* sql_module =
        "CREATE TABLE IF NOT EXISTS modules ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT, path TEXT, platform TEXT, type TEXT, description TEXT,"
        "API TEXT, mode INTEGER, loud INTEGER, output TEXT"
        ");";

    char* err;
    if (sqlite3_exec(db, sql_module, 0, 0, &err) != SQLITE_OK) {
        std::cerr << "MODULE TABLE ERROR: " << err << std::endl;
        sqlite3_free(err);
        return false;
    }

    return true;
}

// Add Module مع التحقق الداخلي
bool ModuD::add(Module m) {
    if (!db) return false;
    if (!m.mode) m.loud = false; //if mode is passive loud is flase
    //this is for the developer don't need it in the run programme
    const char* sql =
        "INSERT INTO modules(name,path,platform,type,description,API,mode,loud,output)"
        " VALUES(?,?,?,?,?,?,?,?,?)";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, m.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, m.path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, m.platform.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, m.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, m.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, m.API.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, m.mode ? 1 : 0);
    sqlite3_bind_int(stmt, 8, m.loud ? 1 : 0);
    sqlite3_bind_text(stmt, 9, m.output.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);   BackupManager::backupDatabase(db_path);
    return ok;
}

bool ModuD::del(int id) {
    if (!db) return false;
    const char* sql = "DELETE FROM modules WHERE id=?";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);    BackupManager::backupDatabase(db_path);
    return ok;
}

std::vector<Module> ModuD::getAll() {
    std::vector<Module> list;
    if (!db) return list;

    const char* sql = "SELECT * FROM modules";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Module m;
        m.id = sqlite3_column_int(stmt, 0);
        m.name = (char*)sqlite3_column_text(stmt, 1);
        m.path = (char*)sqlite3_column_text(stmt, 2);
        m.platform = (char*)sqlite3_column_text(stmt, 3);
        m.type = (char*)sqlite3_column_text(stmt, 4);
        m.description = (char*)sqlite3_column_text(stmt, 5);
        m.API = (char*)sqlite3_column_text(stmt, 6);
        m.mode = sqlite3_column_int(stmt, 7) ? true : false;
        m.loud = sqlite3_column_int(stmt, 8) ? true : false;
        m.output = (char*)sqlite3_column_text(stmt, 9);
        list.push_back(m);
    }

    sqlite3_finalize(stmt);
    return list;
}

ModuleResults ModuD::getWhere(const std::vector<std::string>& columns,
                               const std::vector<std::string>& values) {
    ModuleResults results;
    if (!db) return results;
    if (columns.size() != values.size() || columns.empty()) return results;

    std::stringstream ss;
    ss << "SELECT * FROM modules WHERE ";
    for (size_t i = 0; i < columns.size(); i++) {
        ss << columns[i] << "=?";
        if (i < columns.size() - 1) ss << " AND ";
    }

    std::string sql = ss.str();
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return results;

    for (size_t i = 0; i < values.size(); i++)
        sqlite3_bind_text(stmt, i + 1, values[i].c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Module m;
        m.id = sqlite3_column_int(stmt, 0);
        m.name = (char*)sqlite3_column_text(stmt, 1);
        m.path = (char*)sqlite3_column_text(stmt, 2);
        m.platform = (char*)sqlite3_column_text(stmt, 3);
        m.type = (char*)sqlite3_column_text(stmt, 4);
        m.description = (char*)sqlite3_column_text(stmt, 5);
        m.API = (char*)sqlite3_column_text(stmt, 6);
        m.mode = sqlite3_column_int(stmt, 7) ? true : false;
        m.loud = sqlite3_column_int(stmt, 8) ? true : false;
        m.output = (char*)sqlite3_column_text(stmt, 9);
        results.items.push_back(m);
    }

    sqlite3_finalize(stmt);
    return results;
}
//creating backup and update it every add/del
void BackupManager::backupDatabase(const std::string& originalPath) {
    ConfigManager cfgM("../../config/config.json");
    cfgM.load();
    std::string backupPath =cfgM.get()["App"]["database"]["backup_path"];
    try {
        if (std::filesystem::exists(backupPath)) {
            std::filesystem::remove(backupPath);
        }
        std::filesystem::copy_file(originalPath, backupPath, std::filesystem::copy_options::overwrite_existing);
    }
    catch (const std::exception& e) {
        // ===== PLACEHOLDER FOR FUTURE ERROR HANDLER =====
        std::cout<<"for now now there is error 1"<<std::endl;//debugging for now
    }
    catch (...) {
        // ===== PLACEHOLDER FOR FUTURE ERROR HANDLER =====
              std::cout<<"for now now there is error2"<<std::endl; //debugging for now
    }
}