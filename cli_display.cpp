/*
 *  tguide — cli_display.cpp
 *  CLI display commands: --tool, --vuln, --category
 *
 *  written by voidoxin
 */

#include "cli_display.h"
#include "cli_export.h"
#include "L0-core/include/DatabaseManager.h"
#include "L0-core/include/UserDataManager.h"
#include "L1-services/includes/svc_savedScripts.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <cctype>
#include <limits>
#include <sstream>
#include <filesystem>

// ==============================================================
// Levenshtein distance (case-insensitive)
// ==============================================================
static int editDistance(const std::string& s1, const std::string& s2) {
    size_t n = s1.size();
    size_t m = s2.size();
    // Use two-row optimization for memory efficiency
    std::vector<int> prev(m + 1);
    std::vector<int> curr(m + 1);
    for (size_t j = 0; j <= m; ++j)
        prev[j] = static_cast<int>(j);
    for (size_t i = 1; i <= n; ++i) {
        curr[0] = static_cast<int>(i);
        for (size_t j = 1; j <= m; ++j) {
            int cost = (std::tolower(static_cast<unsigned char>(s1[i - 1])) ==
                        std::tolower(static_cast<unsigned char>(s2[j - 1])))
                           ? 0
                           : 1;
            curr[j] = std::min({prev[j] + 1, curr[j - 1] + 1, prev[j - 1] + cost});
        }
        prev.swap(curr);
    }
    return prev[m];
}

// ==============================================================
// Fuzzy find a tool name by edit distance
// ==============================================================
static std::string fuzzyFindTool(ToolD& db, const std::string& query) {
    auto allTools = db.getAll();
    int minDist   = std::numeric_limits<int>::max();
    std::string bestMatch;

    int queryLen   = static_cast<int>(query.size());
    int threshold  = std::max(3, queryLen / 2);

    for (const auto& t : allTools.items) {
        int dist = editDistance(query, t.name);
        if (dist < minDist) {
            minDist    = dist;
            bestMatch  = t.name;
        }
    }

    if (minDist <= threshold && !bestMatch.empty())
        return bestMatch;
    return {};
}

// ==============================================================
// Filter criteria parsing
// Format: field=value1,value2,...
// ==============================================================
struct FilterCriteria {
    std::string field;
    std::vector<std::string> values;
};

static FilterCriteria parseFilter(const std::string& filterArg) {
    FilterCriteria fc;
    auto eqPos = filterArg.find('=');
    if (eqPos == std::string::npos || eqPos == 0 || eqPos == filterArg.size() - 1)
        return fc; // invalid format or empty field/values

    fc.field = filterArg.substr(0, eqPos);
    // Normalise field to lowercase
    for (auto& c : fc.field) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    std::string valStr = filterArg.substr(eqPos + 1);
    std::istringstream ss(valStr);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Trim whitespace
        auto start = token.find_first_not_of(" \t\r\n");
        auto end   = token.find_last_not_of(" \t\r\n");
        if (start != std::string::npos)
            token = token.substr(start, end - start + 1);
        if (!token.empty())
            fc.values.push_back(token);
    }
    return fc;
}

// Allowed filter fields for vulnerabilities
static const std::set<std::string> ALLOWED_VULN_FIELDS = {
    "severity", "access", "platform", "name",
    "discovered_date", "discoverer", "service", "danger"
};

// ==============================================================
// Check if a value matches any of the filter values (case-insensitive)
// ==============================================================
static bool matchesFilter(const std::string& value, const std::vector<std::string>& filterValues) {
    if (filterValues.empty()) return true;
    for (const auto& fv : filterValues) {
        if (value.size() == fv.size()) {
            // Exact case-insensitive comparison
            bool match = true;
            for (size_t k = 0; k < value.size(); ++k) {
                if (std::tolower(static_cast<unsigned char>(value[k])) !=
                    std::tolower(static_cast<unsigned char>(fv[k]))) {
                    match = false;
                    break;
                }
            }
            if (match) return true;
        }
    }
    return false;
}

// ==============================================================
// Print tool information
// ==============================================================
static void printToolInfo(const Tool& tool, bool showFlags, bool showDesc,
                          bool showTemplates,
                          const std::string& dbPath) {
    // Header
    std::cout << "=== " << tool.name << " ===" << std::endl;

    // When NO filters are specified, show everything
    if (!showFlags && !showDesc && !showTemplates) {
        showFlags    = true;
        showDesc     = true;
        showTemplates = true;
    }

    // Description section
    if (showDesc) {
        if (!tool.short_desc.empty())
            std::cout << "Description: " << tool.short_desc << std::endl;
        if (!tool.description.empty())
            std::cout << tool.description << std::endl;
        if (!tool.short_desc.empty() || !tool.description.empty())
            std::cout << std::endl;
    }

    // Flags section
    if (showFlags) {
        ToolFlagD flagDb(dbPath, DirectOpen{});
        ToolFlagResults flags = flagDb.getWhere(tool.id);
        if (!flags.items.empty()) {
            std::cout << "Flags:" << std::endl;
            // Find longest flag name for alignment padding
            size_t maxLen = 0;
            for (const auto& f : flags.items)
                maxLen = std::max(maxLen, f.name.size());
            size_t padding = std::min(maxLen + 4, size_t(20));
            for (const auto& f : flags.items) {
                std::cout << "  " << f.name;
                size_t spaces = (f.name.size() < padding)
                                    ? padding - f.name.size()
                                    : 2;
                for (size_t s = 0; s < spaces; ++s) std::cout << ' ';
                std::cout << f.description << std::endl;
            }
            std::cout << std::endl;
        }
    }

    // Templates section
    if (showTemplates) {
        TemplateD templDb(dbPath, DirectOpen{});
        TemplateResults templates = templDb.getWhere(tool.id);
        if (!templates.items.empty()) {
            std::cout << "Templates:" << std::endl;
            for (const auto& t : templates.items) {
                std::cout << "  " << t.template_name << ": " << t.description
                          << std::endl;
            }
            std::cout << std::endl;
        }
    }
}

// ==============================================================
// Tool command handler
// ==============================================================
static void handleToolCommand(const std::string& dbPath,
                              const std::string& toolArg,
                              bool showFlags, bool showDesc,
                              bool showTemplates,
                              const std::string& filterArg) {
    ToolD db(dbPath, DirectOpen{});

    // Parse comma-separated tool names
    std::vector<std::string> toolNames;
    std::istringstream ss(toolArg);
    std::string token;
    while (std::getline(ss, token, ',')) {
        // Trim whitespace
        auto start = token.find_first_not_of(" \t\r\n");
        auto end   = token.find_last_not_of(" \t\r\n");
        if (start != std::string::npos)
            token = token.substr(start, end - start + 1);
        if (!token.empty())
            toolNames.push_back(token);
    }

    if (toolNames.empty()) {
        std::cerr << "Error: --tool requires at least one tool name." << std::endl;
        return;
    }

    // Parse filter criteria (for narrowing tool selection)
    FilterCriteria fc;
    bool hasFilter = !filterArg.empty();
    if (hasFilter) {
        fc = parseFilter(filterArg);
        if (fc.field.empty()) {
            std::cerr << "Warning: Invalid filter format '" << filterArg
                      << "'. Expected field=value." << std::endl;
            hasFilter = false;
        }
    }

    // Process each tool name
    for (const auto& name : toolNames) {
        // Exact match by name
        ToolResults results = db.getWhere({"name"}, {name});
        if (results.items.empty()) {
            // Fuzzy search
            std::string suggestion = fuzzyFindTool(db, name);
            if (!suggestion.empty()) {
                std::cout << "Tool '" << name << "' not found. Did you mean '"
                          << suggestion << "'?" << std::endl;
            } else {
                std::cout << "Tool '" << name << "' not found." << std::endl;
            }
            continue;
        }

        bool anyPrinted = false;
        for (const auto& tool : results.items) {
            // Apply optional filter
            if (hasFilter && !fc.values.empty()) {
                // Get the tool's field value
                std::string fieldVal;
                if (fc.field == "name")        fieldVal = tool.name;
                else if (fc.field == "category") fieldVal = tool.category;
                else if (fc.field == "short_desc") fieldVal = tool.short_desc;
                else if (fc.field == "description") fieldVal = tool.description;
                else {
                    std::cerr << "Warning: Unknown filter field '" << fc.field
                              << "' for tools. Skipping filter." << std::endl;
                    // Don't skip the tool — just ignore unknown field
                }

                if (!fc.field.empty() && !matchesFilter(fieldVal, fc.values))
                    continue; // Skip this tool if it doesn't match
            }

            printToolInfo(tool, showFlags, showDesc, showTemplates, dbPath);
            anyPrinted = true;
        }
        if (hasFilter && !anyPrinted) {
            std::cout << "No tools match the given filter criteria." << std::endl;
        }
    }
}

// ==============================================================
// Vulnerability command handler
// ==============================================================
static void handleVulnCommand(const std::string& dbPath,
                              const std::string& filterArg) {
    VulnD db(dbPath, DirectOpen{});

    std::vector<Vulnerability> vulns;

    if (filterArg.empty()) {
        // No filter — get all
        vulns = db.getAll();
    } else {
        auto fc = parseFilter(filterArg);
        if (fc.field.empty()) {
            std::cerr << "Warning: Invalid filter format '" << filterArg
                      << "'. Showing all vulnerabilities." << std::endl;
            vulns = db.getAll();
        } else {
            // Validate field against allowed vulnerability columns
            if (ALLOWED_VULN_FIELDS.find(fc.field) == ALLOWED_VULN_FIELDS.end()) {
                std::cerr << "Warning: Unknown filter field '" << fc.field
                          << "'. Showing all vulnerabilities." << std::endl;
                vulns = db.getAll();
            } else {
                // Get all and filter in-memory (supports multi-value OR)
                auto all = db.getAll();
                for (const auto& v : all) {
                    std::string fieldVal;
                    if (fc.field == "severity")        fieldVal = v.severity;
                    else if (fc.field == "access")     fieldVal = v.access;
                    else if (fc.field == "platform")   fieldVal = v.platform;
                    else if (fc.field == "name")       fieldVal = v.name;
                    else if (fc.field == "discovered_date") fieldVal = v.discovered_date;
                    else if (fc.field == "discoverer") fieldVal = v.discoverer;
                    else if (fc.field == "service")    fieldVal = v.service;
                    else if (fc.field == "danger")     fieldVal = v.danger;

                    if (matchesFilter(fieldVal, fc.values))
                        vulns.push_back(v);
                }
            }
        }
    }

    // Sort by name
    std::sort(vulns.begin(), vulns.end(),
              [](const Vulnerability& a, const Vulnerability& b) {
                  return a.name < b.name;
              });

    if (vulns.empty()) {
        std::cout << "No vulnerabilities found." << std::endl;
        return;
    }

    for (const auto& v : vulns) {
        std::cout << "=== " << v.name << " ===" << std::endl;
        if (!v.metasploit.empty())
            std::cout << "  Metasploit: " << v.metasploit << std::endl;
        if (!v.severity.empty())
            std::cout << "  Severity: " << v.severity << std::endl;
        if (!v.access.empty())
            std::cout << "  Access: " << v.access << std::endl;
        if (!v.platform.empty())
            std::cout << "  Platform: " << v.platform << std::endl;
        if (!v.service.empty())
            std::cout << "  Service: " << v.service << std::endl;
        if (!v.discovered_date.empty()) {
            std::cout << "  Discovered: " << v.discovered_date;
            if (!v.discoverer.empty())
                std::cout << " by " << v.discoverer;
            std::cout << std::endl;
        }
        if (!v.description.empty())
            std::cout << "  Description: " << v.description << std::endl;
        if (!v.danger.empty())
            std::cout << "  Danger: " << v.danger << std::endl;
        std::cout << std::endl;
    }
}

// ==============================================================
// Category command handler
// ==============================================================
static void handleCategoryCommand(const std::string& dbPath,
                                  const std::string& categoryArg) {
    ToolD db(dbPath, DirectOpen{});

    // Case-insensitive category lookup: try exact match first, then case-insensitive
    ToolResults results = db.getWhere({"category"}, {categoryArg});
    if (results.items.empty()) {
        // Try case-insensitive by searching all tools
        auto allTools = db.getAll();
        for (const auto& t : allTools.items) {
            if (t.category.size() == categoryArg.size()) {
                bool match = true;
                for (size_t k = 0; k < categoryArg.size(); ++k) {
                    if (std::tolower(static_cast<unsigned char>(t.category[k])) !=
                        std::tolower(static_cast<unsigned char>(categoryArg[k]))) {
                        match = false;
                        break;
                    }
                }
                if (match)
                    results.items.push_back(t);
            }
        }
    }

    if (results.items.empty()) {
        std::cout << "No tools found in category '" << categoryArg << "'."
                  << std::endl;
        return;
    }

    // Sort by name
    std::sort(results.items.begin(), results.items.end(),
              [](const Tool& a, const Tool& b) { return a.name < b.name; });

    std::cout << "=== " << results.items[0].category << " ===" << std::endl;
    for (const auto& t : results.items) {
        std::cout << "  " << t.name;
        if (!t.short_desc.empty())
            std::cout << " — " << t.short_desc;
        std::cout << std::endl;
    }
    std::cout << std::endl;
}

// ==============================================================
// Main dispatch — called after bootstrap, before UI start
// ==============================================================
int runDisplayCommand(const ParsedArgs& args, const std::string& dbPath) {
    bool hasExport = !args.exportTextArg.empty() || !args.exportJsonArg.empty()
                  || !args.exportYamlArg.empty() || !args.exportCsvArg.empty();

    // ── Saved scripts command ────────────────────────────
    if (args.savedScripts) {
        auto scripts = SvcSavedScripts::getAllScripts();
        // Display the scripts
        if (scripts.empty()) {
            std::cout << "No saved scripts found.\n"
                      << "Use the interactive UI to save scripts.\n";
        } else {
            std::cout << "=== Saved Scripts ===\n";
            for (const auto& s : scripts) {
                std::cout << "  [" << s.id << "] " << s.name << "\n";
                if (!s.path.empty())
                    std::cout << "       Path: " << s.path << "\n";
                if (!s.note.empty())
                    std::cout << "       Note: " << s.note << "\n";
                std::cout << "\n";
            }
            std::cout << scripts.size() << " script(s) total.\n";
        }

        // Export (uses the already-fetched scripts vector)
        bool ok = true;
        if (!args.exportTextArg.empty()) ok &= exportScriptsToText(scripts, args.exportTextArg);
        if (!args.exportJsonArg.empty()) ok &= exportScriptsToJson(scripts, args.exportJsonArg);
        if (!args.exportYamlArg.empty()) ok &= exportScriptsToYaml(scripts, args.exportYamlArg);
        if (!args.exportCsvArg.empty())  ok &= exportScriptsToCsv(scripts, args.exportCsvArg);
        return ok ? 0 : 1;
    }

    // ── Database-dependent commands (--tool, --vuln, --category) ──
    if (!args.toolArg.empty() || args.vuln || !args.categoryArg.empty()) {
        if (!std::filesystem::exists(dbPath)) {
            std::cerr << "Error: Database file not found at " << dbPath << "\n";
            return 1;
        }
    }

    // Helper lambda: run a display lambda while capturing output
    // Restores cout.rdbuf() immediately after displayFn() completes,
    // with a try/catch guard for exception safety.
    auto captureAndExport = [&](auto displayFn) -> int {
        std::stringstream buffer;
        auto* old = std::cout.rdbuf(buffer.rdbuf());
        try {
            displayFn();
        } catch (...) {
            std::cout.rdbuf(old);
            throw;
        }
        std::cout.rdbuf(old);
        std::string output = buffer.str();
        std::cout << output;

        bool ok = true;
        if (!args.exportTextArg.empty())
            ok &= exportTextContent(output, args.exportTextArg);

        // Structured exports only work with --saved-scripts for now
        if (!args.exportJsonArg.empty()) {
            std::cerr << "Error: JSON export is not yet supported for this command. "
                      << "Use --export-text instead.\n";
            ok = false;
        }
        if (!args.exportYamlArg.empty()) {
            std::cerr << "Error: YAML export is not yet supported for this command. "
                      << "Use --export-text instead.\n";
            ok = false;
        }
        if (!args.exportCsvArg.empty()) {
            std::cerr << "Error: CSV export is not yet supported for this command. "
                      << "Use --export-text instead.\n";
            ok = false;
        }
        return ok ? 0 : 1;
    };

    if (!args.toolArg.empty()) {
        return captureAndExport([&]() {
            handleToolCommand(dbPath, args.toolArg, args.flagsFilter,
                              args.descFilter, args.templatesFilter, args.filterArg);
        });
    }

    if (args.vuln) {
        return captureAndExport([&]() {
            handleVulnCommand(dbPath, args.filterArg);
        });
    }

    if (!args.categoryArg.empty()) {
        return captureAndExport([&]() {
            handleCategoryCommand(dbPath, args.categoryArg);
        });
    }

    // ── Export without base command ──────────────────────
    if (hasExport) {
        std::cerr << "Error: Export flags require a base command "
                  << "(--tool, --vuln, --saved-scripts, or --category).\n";
        return 1;
    }

    // ── Filter without base command ──────────────────────
    if (!args.filterArg.empty()) {
        std::cerr << "Error: --filter requires --tool or --vuln.\n";
        return 1;
    }

    return 0;
}
