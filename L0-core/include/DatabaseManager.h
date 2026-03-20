#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>

struct Vulnerability {
    int id;
    std::string name, metasploit, discovered_date, discoverer;
    std::string severity, access, platform, service, description;
    std::string danger;
};

struct Option {
    int id;                                           int vuln_id;
    std::string name;                                 std::string value;
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

struct VulnResults     { std::vector<Vulnerability> items; };
struct ModuleResults   { std::vector<Module>        items; };
struct ToolResults     { std::vector<Tool>           items; };
struct ToolFlagResults { std::vector<ToolFlag>       items; };
struct TemplateResults { std::vector<Template>       items; };

class BackupManager {
public:
    static void init(const std::string& backupPath);
    static void backupDatabase(const std::string& originalPath);
private:
    static std::string s_backupPath;
};

/*
 * Tag struct for direct DB open — bypasses resolveDatabase().
 * Used by data_adder only. Not for use in the main application.
 *
 * © voidoxin — Tools Guide
 */
struct DirectOpen {};

/*
 * Returns true if a fatal error occurred during database initialization.
 * Must be checked after constructing any database class before proceeding.
 *
 * © voidoxin — Tools Guide
 */
bool DBFatal();

class VulnD {
    std::string db_path;
public:
    VulnD(const std::string& path);
    VulnD(const std::string& path, DirectOpen);   // bypasses resolveDatabase

    bool createTables();

    bool add(const Vulnerability& v);
    bool del(int id);

    bool addOption(const Option& o);
    bool delOption(int option_id);

    std::vector<Vulnerability> getAll();
    std::vector<Option>        getOptions(int vuln_id);

    VulnResults getWhere(const std::vector<std::string>& columns,
                         const std::vector<std::string>& values);
};

class ModuD {
    std::string db_path;
public:
    ModuD(const std::string& path);
    ModuD(const std::string& path, DirectOpen);

    bool createTables();
    bool add(const Module& m);
    bool del(int id);

    std::vector<Module> getAll();

    ModuleResults getWhere(const std::vector<std::string>& columns,
                           const std::vector<std::string>& values);
};

class ToolD {
    std::string db_path;
public:
    ToolD(const std::string& path);
    ToolD(const std::string& path, DirectOpen);

    bool createTables();
    bool add(const Tool& t);
    bool del(int id);

    ToolResults getAll();
};

class ToolFlagD {
    std::string db_path;
public:
    ToolFlagD(const std::string& path);
    ToolFlagD(const std::string& path, DirectOpen);

    bool createTables();
    bool add(const ToolFlag& f);
    bool del(int id);

    ToolFlagResults getWhere(int tool_id);
};

class TemplateD {
    std::string db_path;
public:
    TemplateD(const std::string& path);
    TemplateD(const std::string& path, DirectOpen);

    bool createTables();
    bool add(const Template& t);
    bool del(int id);

    TemplateResults getWhere(int tool_id);
};