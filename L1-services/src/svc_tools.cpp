/*
 *  tguide — svc_tools.cpp
 *  service bridge: UI_tools → ToolD queries
 *
 *  written by voidoxin
 */

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

#include "../../L0-core/include/DatabaseManager.h"
#include "../../L0-core/include/path_resolver.h"
#include "../../L0-core/include/UserDataManager.h"
#include "../includes/svc_generator.h"
#include "../includes/svc_tools.h"

using namespace std;

// ── L0→DTO mappers ────────────────────────────────────────────────────
// These convert L0-core types to L1 DTOs, keeping L0 types internal to L1.

static SvcDTO::ToolDTO toDTO(const Tool& t) {
    return { t.id, t.name, t.category, t.short_desc, t.description, t.flags_all };
}

static std::vector<SvcDTO::ToolDTO> toDTOs(const std::vector<Tool>& tools) {
    std::vector<SvcDTO::ToolDTO> result;
    result.reserve(tools.size());
    for (const auto& t : tools) result.push_back(toDTO(t));
    return result;
}

static SvcDTO::ToolFlagDTO toDTO(const ToolFlag& f) {
    return { f.id, f.tool_id, f.name, f.description, f.loud, f.root, f.protocols };
}

static SvcDTO::TemplateDTO toDTO(const Template& t) {
    return { t.id, t.tool_id, t.template_name, t.description, t.root, t.protocols, t.flag };
}

static SvcDTO::VulnerabilityDTO toDTO(const Vulnerability& v) {
    return { v.id, v.name, v.metasploit, v.discovered_date, v.discoverer,
             v.severity, v.access, v.platform, v.service, v.description, v.danger };
}

namespace SvcTools {
// ── TOOLS BY CATEGORY ───────────────────────────────────────────────────────

vector<SvcDTO::ToolDTO> getToolsByCategory(const string& category) {
    ToolD db(PathResolver::dbFile().string());
    ToolResults res = db.getWhere({"category"}, {category});
    return toDTOs(res.items);
}

// ── SEARCH ──────────────────────────────────────────────────────────────────

// TODO: implement search algorithm
vector<SvcDTO::ToolDTO> searchTools(const string& query) {
    (void)query; return {};
}

// ── CATEGORY LIST (from categories table) ─────────────────────────────────

vector<SvcDTO::CategoryDTO> getCategoryList() {
    CategoryD db(PathResolver::dbFile().string());
    CategoryResults res = db.getAll();

    // Sort by display_order, then alphabetically by name
    sort(res.items.begin(), res.items.end(),
         [](const Category& a, const Category& b) {
             if (a.display_order != b.display_order)
                 return a.display_order < b.display_order;
             return a.name < b.name;
         });

    vector<SvcDTO::CategoryDTO> result;
    result.reserve(res.items.size());
    for (const auto& c : res.items) {
        result.push_back({c.id, c.name, c.description});
    }
    return result;
}

// ── TOOL DETAIL ─────────────────────────────────────────────────────

SvcDTO::ToolDTO getToolById(int id) {
    ToolD db(PathResolver::dbFile().string());
    ToolResults res = db.getWhere({"id"}, {to_string(id)});
    if (res.items.empty()) return {};
    return toDTO(res.items[0]);
}

vector<SvcDTO::ToolFlagDTO> getFlagsByToolId(int toolId) {
    ToolFlagD db(PathResolver::dbFile().string());
    ToolFlagResults res = db.getWhere(toolId);
    vector<SvcDTO::ToolFlagDTO> result;
    result.reserve(res.items.size());
    for (const auto& f : res.items) result.push_back(toDTO(f));
    return result;
}

vector<SvcDTO::TemplateDTO> getTemplatesByToolId(int toolId) {
    TemplateD db(PathResolver::dbFile().string());
    TemplateResults res = db.getWhere(toolId);
    vector<SvcDTO::TemplateDTO> result;
    result.reserve(res.items.size());
    for (const auto& t : res.items) result.push_back(toDTO(t));
    return result;
}

// ── TEMPLATE FILL ────────────────────────────────────────────────────

string buildCommand(const SvcDTO::ToolDTO& tool,
                    const SvcDTO::TemplateDTO& templ,
                    const string& target,
                    const string& port) {
    string cmd;

    // Root prefix
    if (templ.root)
        cmd += "sudo ";

    // Tool name
    cmd += tool.name;

    // Flags — use the tool's concatenated flags_all if the template has flag set
    if (templ.flag && !tool.flags_all.empty())
        cmd += " " + tool.flags_all;

    // Target (always present when this function is called)
    if (!target.empty())
        cmd += " " + target;

    // Port
    // TODO: make port flag configurable per template — different tools use
    // -p (nmap, masscan), --port (curl), -P (some tools), or none
    if (!port.empty())
        cmd += " -p " + port;

    return cmd;
}

int saveTemplateCommand(int toolId, const string& command, const string& note) {
    return UserDataManager::instance().saveCommand(toolId, command, note);
}

// ── VULNERABILITIES ────────────────────────────────────────────────

vector<SvcDTO::VulnerabilityDTO> getAllVulnerabilities() {
    VulnD db(PathResolver::dbFile().string());
    auto items = db.getAll();
    sort(items.begin(), items.end(),
         [](const Vulnerability& a, const Vulnerability& b) {
             return a.name < b.name;
         });
    vector<SvcDTO::VulnerabilityDTO> result;
    result.reserve(items.size());
    for (const auto& v : items) result.push_back(toDTO(v));
    return result;
}

vector<SvcDTO::VulnerabilityDTO> searchVulnerabilities(const string& query) {
    VulnD db(PathResolver::dbFile().string());
    vector<SvcDTO::VulnerabilityDTO> result;

    // Search by name
    {
        VulnResults res = db.getWhere({"name"}, {query});
        for (const auto& v : res.items) result.push_back(toDTO(v));
    }

    // Search by metasploit name
    {
        VulnResults res = db.getWhere({"metasploit_name"}, {query});
        for (const auto& v : res.items) result.push_back(toDTO(v));
    }

    // Deduplicate by id
    sort(result.begin(), result.end(),
         [](const SvcDTO::VulnerabilityDTO& a, const SvcDTO::VulnerabilityDTO& b) {
             return a.id < b.id;
         });
    auto last = unique(result.begin(), result.end(),
                       [](const SvcDTO::VulnerabilityDTO& a, const SvcDTO::VulnerabilityDTO& b) {
                           return a.id == b.id;
                       });
    result.erase(last, result.end());

    sort(result.begin(), result.end(),
         [](const SvcDTO::VulnerabilityDTO& a, const SvcDTO::VulnerabilityDTO& b) {
             return a.name < b.name;
         });
    return result;
}

vector<SvcDTO::VulnerabilityDTO> filterVulnerabilities(const string& column,
                                                        const string& value) {
    static const set<string> ALLOWED = {"severity", "access", "platform"};
    if (ALLOWED.find(column) == ALLOWED.end()) return {};

    VulnD db(PathResolver::dbFile().string());
    VulnResults res = db.getWhere({column}, {value});
    vector<SvcDTO::VulnerabilityDTO> result;
    result.reserve(res.items.size());
    for (const auto& v : res.items) result.push_back(toDTO(v));
    return result;
}

vector<string> getDistinctValues(const string& column) {
    VulnD db(PathResolver::dbFile().string());
    auto all = db.getAll();
    set<string> seen;
    vector<string> result;
    for (const auto& v : all) {
        string val;
        if (column == "severity")       val = v.severity;
        else if (column == "access")    val = v.access;
        else if (column == "platform")  val = v.platform;
        else continue;
        if (val.empty()) continue;
        string key = val;
        transform(key.begin(), key.end(), key.begin(),
                  [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (seen.insert(key).second)
            result.push_back(val);
    }
    sort(result.begin(), result.end());
    return result;
}
} // namespace SvcTools
