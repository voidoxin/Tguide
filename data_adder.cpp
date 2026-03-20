#include <iostream>
#include <string>
#include <limits>
#include <algorithm>
#include "../include/config_manager.h"
#include "DatabaseManager.h"

static void clearInput() {
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static std::string prompt(const std::string& label) {
    std::string val;
    std::cout << "  " << label << ": ";
    std::getline(std::cin, val);
    return val;
}

static bool promptBool(const std::string& label) {
    std::string val;
    std::cout << "  " << label << " (y/n): ";
    std::getline(std::cin, val);
    return val == "y" || val == "Y";
}

static int promptInt(const std::string& label) {
    int val;
    std::cout << "  " << label << ": ";
    std::cin >> val;
    clearInput();
    return val;
}

// ==================== VIEW ====================

static void viewLastVulns(const std::string& db_path) {
    int n = promptInt("How many to show");
    VulnD db(db_path);
    auto all = db.getAll();
    int start = std::max(0, (int)all.size() - n);
    std::cout << "\n--- Last " << n << " Vulnerabilities ---\n";
    for (int i = start; i < (int)all.size(); i++) {
        auto& v = all[i];
        std::cout << "  [" << v.id << "] " << v.name
                  << " | " << v.severity
                  << " | " << v.service
                  << " | " << v.platform << "\n"
                  << "       metasploit: " << v.metasploit << "\n"
                  << "       danger: "     << v.danger     << "\n"
                  << "       desc: "       << v.description << "\n\n";
    }
}

static void viewLastModules(const std::string& db_path) {
    int n = promptInt("How many to show");
    ModuD db(db_path);
    auto all = db.getAll();
    int start = std::max(0, (int)all.size() - n);
    std::cout << "\n--- Last " << n << " Modules ---\n";
    for (int i = start; i < (int)all.size(); i++) {
        auto& m = all[i];
        std::cout << "  [" << m.id << "] " << m.name
                  << " | " << m.platform
                  << " | type: " << m.type << "\n"
                  << "       path: " << m.path << "\n"
                  << "       mode: " << (m.mode ? "active" : "passive")
                  << " | loud: "     << (m.loud ? "yes" : "no") << "\n"
                  << "       desc: " << m.description << "\n\n";
    }
}

static void viewLastTools(const std::string& db_path) {
    int n = promptInt("How many to show");
    ToolD db(db_path);
    auto all = db.getAll().items;
    int start = std::max(0, (int)all.size() - n);
    std::cout << "\n--- Last " << n << " Tools ---\n";
    for (int i = start; i < (int)all.size(); i++) {
        auto& t = all[i];
        std::cout << "  [" << t.id << "] " << t.name << "\n"
                  << "       desc: "  << t.description << "\n"
                  << "       flags: " << t.flags_all   << "\n\n";
    }
}

static void viewLastToolFlags(const std::string& db_path) {
    ToolD tdb(db_path);
    auto tools = tdb.getAll().items;
    if (tools.empty()) { std::cout << "  No tools found.\n"; return; }
    for (auto& t : tools)
        std::cout << "  [" << t.id << "] " << t.name << "\n";
    int tool_id = promptInt("Tool ID");
    int n       = promptInt("How many flags to show");
    ToolFlagD fdb(db_path);
    auto all = fdb.getWhere(tool_id).items;
    int start = std::max(0, (int)all.size() - n);
    std::cout << "\n--- Last " << n << " Flags ---\n";
    for (int i = start; i < (int)all.size(); i++) {
        auto& f = all[i];
        std::cout << "  [" << f.id << "] " << f.name
                  << " | loud: "      << f.loud
                  << " | root: "      << (f.root ? "yes" : "no")
                  << " | protocols: " << f.protocols << "\n"
                  << "       desc: "  << f.description << "\n\n";
    }
}

static void viewLastTemplates(const std::string& db_path) {
    ToolD tdb(db_path);
    auto tools = tdb.getAll().items;
    if (tools.empty()) { std::cout << "  No tools found.\n"; return; }
    for (auto& t : tools)
        std::cout << "  [" << t.id << "] " << t.name << "\n";
    int tool_id = promptInt("Tool ID");
    int n       = promptInt("How many templates to show");
    TemplateD tmdb(db_path);
    auto all = tmdb.getWhere(tool_id).items;
    int start = std::max(0, (int)all.size() - n);
    std::cout << "\n--- Last " << n << " Templates ---\n";
    for (int i = start; i < (int)all.size(); i++) {
        auto& t = all[i];
        std::cout << "  [" << t.id << "] " << t.template_name
                  << " | root: "      << (t.root ? "yes" : "no")
                  << " | protocols: " << t.protocols << "\n"
                  << "       desc: "  << t.description << "\n\n";
    }
}

// ==================== DELETE ====================

static void deleteVulnerability(const std::string& db_path) {
    VulnD db(db_path);
    auto all = db.getAll();
    if (all.empty()) { std::cout << "  No vulnerabilities.\n"; return; }
    for (auto& v : all)
        std::cout << "  [" << v.id << "] " << v.name << "\n";
    int id = promptInt("ID to delete");
    if (db.del(id))
        std::cout << "  [+] Deleted.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void deleteVulnOption(const std::string& db_path) {
    VulnD db(db_path);
    auto vulns = db.getAll();
    if (vulns.empty()) { std::cout << "  No vulnerabilities.\n"; return; }
    for (auto& v : vulns)
        std::cout << "  [" << v.id << "] " << v.name << "\n";
    int vuln_id = promptInt("Vuln ID");
    auto opts = db.getOptions(vuln_id);
    if (opts.empty()) { std::cout << "  No options for this vulnerability.\n"; return; }
    for (auto& o : opts)
        std::cout << "  [" << o.id << "] " << o.name << " = " << o.value << "\n";
    int id = promptInt("Option ID to delete");
    if (db.delOption(id))
        std::cout << "  [+] Deleted.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void deleteModule(const std::string& db_path) {
    ModuD db(db_path);
    auto all = db.getAll();
    if (all.empty()) { std::cout << "  No modules.\n"; return; }
    for (auto& m : all)
        std::cout << "  [" << m.id << "] " << m.name << "\n";
    int id = promptInt("ID to delete");
    if (db.del(id))
        std::cout << "  [+] Deleted.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void deleteTool(const std::string& db_path) {
    ToolD db(db_path);
    auto all = db.getAll().items;
    if (all.empty()) { std::cout << "  No tools.\n"; return; }
    for (auto& t : all)
        std::cout << "  [" << t.id << "] " << t.name << "\n";
    int id = promptInt("ID to delete");
    if (db.del(id))
        std::cout << "  [+] Deleted. (flags and templates deleted automatically via FK)\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void deleteToolFlag(const std::string& db_path) {
    ToolD tdb(db_path);
    auto tools = tdb.getAll().items;
    if (tools.empty()) { std::cout << "  No tools.\n"; return; }
    for (auto& t : tools)
        std::cout << "  [" << t.id << "] " << t.name << "\n";
    int tool_id = promptInt("Tool ID");
    ToolFlagD fdb(db_path);
    auto flags = fdb.getWhere(tool_id).items;
    if (flags.empty()) { std::cout << "  No flags for this tool.\n"; return; }
    for (auto& f : flags)
        std::cout << "  [" << f.id << "] " << f.name << "\n";
    int id = promptInt("Flag ID to delete");
    if (fdb.del(id))
        std::cout << "  [+] Deleted.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void deleteTemplate(const std::string& db_path) {
    ToolD tdb(db_path);
    auto tools = tdb.getAll().items;
    if (tools.empty()) { std::cout << "  No tools.\n"; return; }
    for (auto& t : tools)
        std::cout << "  [" << t.id << "] " << t.name << "\n";
    int tool_id = promptInt("Tool ID");
    TemplateD tmdb(db_path);
    auto tmpls = tmdb.getWhere(tool_id).items;
    if (tmpls.empty()) { std::cout << "  No templates for this tool.\n"; return; }
    for (auto& t : tmpls)
        std::cout << "  [" << t.id << "] " << t.template_name << "\n";
    int id = promptInt("Template ID to delete");
    if (tmdb.del(id))
        std::cout << "  [+] Deleted.\n";
    else
        std::cout << "  [-] Failed.\n";
}

// ==================== ADD ====================

static void addVulnerability(const std::string& db_path) {
    std::cout << "\n--- Add Vulnerability ---\n";
    Vulnerability v;
    v.id              = 0;
    v.name            = prompt("Name");
    v.metasploit      = prompt("Metasploit module");
    v.discovered_date = prompt("Discovered date");
    v.discoverer      = prompt("Discoverer");
    v.severity        = prompt("Severity (critical/high/medium/low)");
    v.access          = prompt("Access (remote/local)");
    v.platform        = prompt("Platform");
    v.service         = prompt("Service");
    v.description     = prompt("Description");
    v.danger          = prompt("Danger");
    VulnD db(db_path);
    if (db.add(v))
        std::cout << "  [+] Vulnerability added.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void addVulnOption(const std::string& db_path) {
    std::cout << "\n--- Add Option to Vulnerability ---\n";
    VulnD db(db_path);
    auto vulns = db.getAll();
    if (vulns.empty()) { std::cout << "  No vulnerabilities found.\n"; return; }
    for (auto& v : vulns)
        std::cout << "  [" << v.id << "] " << v.name << "\n";
    Option o;
    o.id      = 0;
    o.vuln_id = promptInt("Vuln ID");
    o.name    = prompt("Option name");
    o.value   = prompt("Option value");
    if (db.addOption(o))
        std::cout << "  [+] Option added.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void addModule(const std::string& db_path) {
    std::cout << "\n--- Add Module ---\n";
    Module m;
    m.id          = 0;
    m.name        = prompt("Name");
    m.path        = prompt("Path");
    m.platform    = prompt("Platform");
    m.type        = prompt("Type");
    m.description = prompt("Description");
    m.API         = prompt("API");
    m.mode        = promptBool("Active mode");
    m.loud        = m.mode ? promptBool("Loud") : false;
    m.output      = prompt("Output");
    ModuD db(db_path);
    if (db.add(m))
        std::cout << "  [+] Module added.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void addTool(const std::string& db_path) {
    std::cout << "\n--- Add Tool ---\n";
    Tool t;
    t.id          = 0;
    t.name        = prompt("Name");
    t.description = prompt("Description");
    t.flags_all   = prompt("All flags (summary)");
    ToolD db(db_path);
    if (db.add(t))
        std::cout << "  [+] Tool added.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void addToolFlag(const std::string& db_path) {
    std::cout << "\n--- Add Tool Flag ---\n";
    ToolD tdb(db_path);
    auto tools = tdb.getAll().items;
    if (tools.empty()) { std::cout << "  No tools found. Add a tool first.\n"; return; }
    for (auto& t : tools)
        std::cout << "  [" << t.id << "] " << t.name << "\n";
    ToolFlag f;
    f.id          = 0;
    f.tool_id     = promptInt("Tool ID");
    f.name        = prompt("Flag name");
    f.description = prompt("Description");
    f.loud        = prompt("Loud (yes/no/sometimes)");
    f.root        = promptBool("Requires root");
    f.protocols   = prompt("Protocols");
    ToolFlagD fdb(db_path);
    if (fdb.add(f))
        std::cout << "  [+] Flag added.\n";
    else
        std::cout << "  [-] Failed.\n";
}

static void addTemplate(const std::string& db_path) {
    std::cout << "\n--- Add Template ---\n";
    ToolD tdb(db_path);
    auto tools = tdb.getAll().items;
    if (tools.empty()) { std::cout << "  No tools found. Add a tool first.\n"; return; }
    for (auto& t : tools)
        std::cout << "  [" << t.id << "] " << t.name << "\n";
    Template tmpl;
    tmpl.id            = 0;
    tmpl.tool_id       = promptInt("Tool ID");
    tmpl.template_name = prompt("Template name");
    tmpl.description   = prompt("Description");
    tmpl.root          = promptBool("Requires root");
    tmpl.protocols     = prompt("Protocols");
    tmpl.flag          = promptBool("Has flag");
    TemplateD tmdb(db_path);
    if (tmdb.add(tmpl))
        std::cout << "  [+] Template added.\n";
    else
        std::cout << "  [-] Failed.\n";
}

// ==================== INIT ====================

static bool initDatabase(const std::string& db_path) {
    bool ok = true;
    std::cout << "[*] Creating tables...\n";
    { VulnD     db(db_path); ok &= db.createTables(); }
    { ModuD     db(db_path); ok &= db.createTables(); }
    { ToolD     db(db_path); ok &= db.createTables(); }
    { ToolFlagD db(db_path); ok &= db.createTables(); }
    { TemplateD db(db_path); ok &= db.createTables(); }
    if (ok) std::cout << "[+] All tables ready.\n";
    else    std::cout << "[-] Some tables failed.\n";
    return ok;
}

// ==================== MAIN ====================

int main() {
    ConfigManager cfg("config/config.json");
    std::string db_path     = cfg.get()["App"]["database"]["db_path"];
    std::string backup_path = cfg.get()["App"]["database"]["backup_path"];

    BackupManager::init(backup_path);

    if (!initDatabase(db_path)) {
        std::cerr << "[-] Database initialization failed. Exiting.\n";
        return 1;
    }

    while (true) {
        std::cout << "\n========== MENU ==========\n";
        std::cout << "  --- Add ---\n";
        std::cout << "  1. Add Vulnerability\n";
        std::cout << "  2. Add Vulnerability Option\n";
        std::cout << "  3. Add Module\n";
        std::cout << "  4. Add Tool\n";
        std::cout << "  5. Add Tool Flag\n";
        std::cout << "  6. Add Template\n";
        std::cout << "  --- View ---\n";
        std::cout << "  7.  View last N Vulnerabilities\n";
        std::cout << "  8.  View last N Modules\n";
        std::cout << "  9.  View last N Tools\n";
        std::cout << "  10. View last N Tool Flags\n";
        std::cout << "  11. View last N Templates\n";
        std::cout << "  --- Delete ---\n";
        std::cout << "  12. Delete Vulnerability\n";
        std::cout << "  13. Delete Vulnerability Option\n";
        std::cout << "  14. Delete Module\n";
        std::cout << "  15. Delete Tool\n";
        std::cout << "  16. Delete Tool Flag\n";
        std::cout << "  17. Delete Template\n";
        std::cout << "  ---\n";
        std::cout << "  0. Exit\n";
        std::cout << "==========================\n";
        std::cout << "Choice: ";

        int choice;
        std::cin >> choice;
        clearInput();

        switch (choice) {
            case 1:  addVulnerability(db_path);  break;
            case 2:  addVulnOption(db_path);     break;
            case 3:  addModule(db_path);         break;
            case 4:  addTool(db_path);           break;
            case 5:  addToolFlag(db_path);       break;
            case 6:  addTemplate(db_path);       break;
            case 7:  viewLastVulns(db_path);     break;
            case 8:  viewLastModules(db_path);   break;
            case 9:  viewLastTools(db_path);     break;
            case 10: viewLastToolFlags(db_path); break;
            case 11: viewLastTemplates(db_path); break;
            case 12: deleteVulnerability(db_path); break;
            case 13: deleteVulnOption(db_path);    break;
            case 14: deleteModule(db_path);        break;
            case 15: deleteTool(db_path);          break;
            case 16: deleteToolFlag(db_path);      break;
            case 17: deleteTemplate(db_path);      break;
            case 0:
                std::cout << "[*] Exiting.\n";
                return 0;
            default:
                std::cout << "  Invalid choice.\n";
        }
    }
}