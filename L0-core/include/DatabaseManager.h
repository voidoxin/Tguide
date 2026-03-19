#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>

// ==================== STRUCTS ====================

struct Vulnerability {
    int id;
    std::string name, metasploit, discovered_date, discoverer;
    std::string severity, access, platform, service, description;
    std::string danger; // جديد
};

struct Option {
    int id;
    int vuln_id;
    std::string name;
    std::string value;
};

struct Module {
    int id;
    std::string name, path, platform, type, description;
    std::string API;
    bool mode;
    bool loud;
    std::string output;

    Module() : mode(false), loud(false) {}
};

struct Tool {
    int id;
    std::string name;
    std::string description;
    std::string flags_all;
};

struct ToolFlag {
    int id;
    int tool_id;
    std::string name;
    std::string description;
    std::string loud;
    bool root;
    std::string protocols;
};

struct Template {
    int id;
    int tool_id;
    std::string template_name;
    std::string description;
    bool root;
    std::string protocols;
    bool flag;
};

// ==================== RESULTS STRUCTS ====================

struct VulnResults { std::vector<Vulnerability> items; };
struct ModuleResults { std::vector<Module> items; };
struct ToolResults { std::vector<Tool> items; };
struct ToolFlagResults { std::vector<ToolFlag> items; };
struct TemplateResults { std::vector<Template> items; };

// ==================== VulnD ====================

class VulnD {
private:
    sqlite3* db;
    std::string db_path;

public:
    VulnD(const std::string& path);
    ~VulnD();

    bool createTables();

    bool add(const Vulnerability& v);
    bool del(int id);

    bool addOption(const Option& o);
    bool delOption(int option_id);

    std::vector<Vulnerability> getAll();
    std::vector<Option> getOptions(int vuln_id);

    VulnResults getWhere(const std::vector<std::string>& columns,
                         const std::vector<std::string>& values);
};

// ==================== ModuD ====================

class ModuD {
private:
    sqlite3* db;
    std::string db_path;

public:
    ModuD(const std::string& path);
    ~ModuD();

    bool createTables();
    bool add(Module m);
    bool del(int id);

    std::vector<Module> getAll();

    ModuleResults getWhere(const std::vector<std::string>& columns,
                           const std::vector<std::string>& values);
};

// ==================== ToolD ====================

class ToolD {
private:
    sqlite3* db;
    std::string db_path;

public:
    ToolD(const std::string& path);
    ~ToolD();

    bool createTables();

    bool add(const Tool& t);
    bool del(int id);

    ToolResults getAll();
};

// ==================== ToolFlagD ====================

class ToolFlagD {
private:
    sqlite3* db;
    std::string db_path;

public:
    ToolFlagD(const std::string& path);
    ~ToolFlagD();

    bool createTables();

    bool add(const ToolFlag& f);
    bool del(int id);

    ToolFlagResults getWhere(int tool_id);
};

// ==================== TemplateD ====================

class TemplateD {
private:
    sqlite3* db;
    std::string db_path;

public:
    TemplateD(const std::string& path);
    ~TemplateD();

    bool createTables();

    bool add(const Template& t);
    bool del(int id);

    TemplateResults getWhere(int tool_id);
};
class BackupManager {
public:
    static void backupDatabase(const std::string& originalPath);
};