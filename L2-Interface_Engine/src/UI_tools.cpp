/*
 *  tguide — UI_tools.cpp
 *  tools entry screen, category browser, tool list per category
 *
 *  written by voidoxin
 */

#include <algorithm>
#include <cctype>
#include <iostream>
#include <string>
#include <vector>

#include "../../L0-core/include/DatabaseManager.h"
#include "../../L0-core/include/path_resolver.h"
#include "../../L0-core/include/UserDataManager.h"
#include "../includes/svc_generator.h"
#include "../includes/svc_strings.h"
#include "../includes/svc_tools.h"
#include "../includes/svc_savedTemplates.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include "../includes/UI_paginator.h"
#include "../includes/UI_tools.h"
#include "../includes/UI_utils.h"

using namespace std;

// ── forward declarations ────────────────────────────────────────────────────
static void showToolDetail(const SvcDTO::ToolDTO& tool);
static void showToolsByCategory(const string& category);
static void showCategories();
static void showSearch();
static void showAllTools();
static void showTemplateFill(const SvcDTO::ToolDTO& tool,
                              const SvcDTO::TemplateDTO& templ);
static void showMyTemplates();
static void addNewTemplate();
static void useSavedTemplate(const SvcDTO::SavedTemplateDTO& st);
static void showVulnerabilities();
static void showModules();

// ==================== HELPERS ====================

// mirrors Paginator::select() first-word extraction for label matching
static vector<string> paginatorLabels(const vector<string>& items) {
    vector<string> labels;
    labels.reserve(items.size());
    for (const auto& item : items) {
        size_t sp = item.find(' ');
        labels.push_back(sp == string::npos ? item : item.substr(0, sp));
    }
    return labels;
}

// returns true if input is a prefix matching 2+ options — distinguishes
// ambiguous from invalid so callers can print the correct inline message
static bool isAmbiguous(const string& input, const vector<string>& opts) {
    string norm = normalize(input);
    if (norm.empty() || toNumber(norm) != -1) return false;
    int count = 0;
    for (const string& opt : opts) {
        string lo = normalize(opt);
        if (lo.size() >= norm.size() && lo.compare(0, norm.size(), norm) == 0) {
            if (++count > 1) return true;
        }
    }
    return false;
}

// ==================== VULNERABILITY HELPERS ====================

static bool isMetasploit(const SvcDTO::ToolDTO& tool) {
    string lo = tool.name;
    transform(lo.begin(), lo.end(), lo.begin(),
              [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lo == "metasploit";
}

static bool isReconNg(const SvcDTO::ToolDTO& tool) {
    string lo = tool.name;
    transform(lo.begin(), lo.end(), lo.begin(),
              [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lo == "recon-ng";
}

// ==================== TOOL DETAIL (STUB) ====================

static void showToolDetail(const SvcDTO::ToolDTO& tool) {
    vector<SvcDTO::ToolFlagDTO> flags     = SvcTools::getFlagsByToolId(tool.id);
    vector<SvcDTO::TemplateDTO> templates = SvcTools::getTemplatesByToolId(tool.id);
    vector<SvcDTO::SavedTemplateDTO> savedTmpls = SvcSavedTemplates::getTemplatesByToolId(tool.id);

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a " + tool.category + " \u203a " + tool.name);
        UI::printDivider();

        // ── description ──
        cout << "\n  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << tool.name
             << (colorsEnabled() ? Color::RESET : "") << "\n\n  "
             << tool.description << "\n\n";

        // ── options banner (always visible) ──
        cout << (colorsEnabled() ? Color::DIM : "");
        if (isMetasploit(tool))
            cout << "  [v] vulnerabilities\n";
        if (isReconNg(tool))
            cout << "  [m] modules\n";
        cout << "  [0] " << Strings::get(StringID::TOOLS_BACK) << "\n";
        cout << (colorsEnabled() ? Color::RESET : "");

        // ── flags ──
        if (!flags.empty()) {
            cout << "\n  " << (colorsEnabled() ? Color::BOLD : "")
                 << "flags:"
                 << (colorsEnabled() ? Color::RESET : "") << "\n";
            for (const auto& f : flags) {
                cout << "    " << (colorsEnabled() ? Color::CYAN : "") << f.name
                     << (colorsEnabled() ? Color::RESET : "");
                if (!f.description.empty())
                    cout << "  \u2014 " << f.description;
                if (!f.protocols.empty())
                    cout << "  (" << f.protocols << ")";
                if (f.root)
                    cout << "  [root]";
                cout << "\n";
            }
        }

        // ── templates ──
        if (!templates.empty()) {
            cout << "\n  " << (colorsEnabled() ? Color::BOLD : "")
                 << "templates:"
                 << (colorsEnabled() ? Color::RESET : "") << "\n";
            for (size_t i = 0; i < templates.size(); i++) {
                const auto& t = templates[i];
                cout << "    [" << (i + 1) << "] "
                     << (colorsEnabled() ? Color::CYAN : "") << t.template_name
                     << (colorsEnabled() ? Color::RESET : "");
                if (!t.description.empty())
                    cout << "  \u2014 " << t.description;
                if (!t.protocols.empty())
                    cout << "  (" << t.protocols << ")";
                if (t.root)
                    cout << "  [root]";
                cout << "\n";
            }
        }

        // ── saved templates ──
        if (!savedTmpls.empty()) {
            if (!templates.empty()) {
                cout << "\n  " << (colorsEnabled() ? Color::DIM : "")
                     << "  \u2014\u2014\u2014 user templates \u2014\u2014\u2014"
                     << (colorsEnabled() ? Color::RESET : "") << "\n";
            }
            for (size_t i = 0; i < savedTmpls.size(); i++) {
                const auto& st = savedTmpls[i];
                int num = static_cast<int>(templates.size() + i + 1);
                cout << "    [" << num << "] "
                     << (colorsEnabled() ? Color::CYAN : "") << st.name
                     << (colorsEnabled() ? Color::RESET : "")
                     << "  (saved)";
                if (!st.description.empty())
                    cout << "  \u2014 " << st.description;
                cout << "\n";
            }
        }

        UI::printDivider();
        cout << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;

        // Check if input is a template number (DB + saved)
        int num = toNumber(input);
        int totalTemplates = static_cast<int>(templates.size() + savedTmpls.size());
        if (num >= 1 && num <= totalTemplates) {
            if (num <= static_cast<int>(templates.size())) {
                showTemplateFill(tool, templates[static_cast<size_t>(num - 1)]);
            } else {
                int savedIdx = num - static_cast<int>(templates.size()) - 1;
                useSavedTemplate(savedTmpls[savedIdx]);
            }
            continue;
        }

        // Metasploit vulnerabilities
        if (isMetasploit(tool) && (input == "v" || input == "V")) {
            showVulnerabilities();
            continue;
        }
        // Recon-ng modules
        if (isReconNg(tool) && (input == "m" || input == "M")) {
            showModules();
            continue;
        }

        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
    }
}

// ==================== TOOL LIST ====================

static void showToolsByCategory(const string& category) {
    vector<SvcDTO::ToolDTO> tools = SvcTools::getToolsByCategory(category);

    if (tools.empty()) {
        cout << "\n  no tools in this category.\n\n";
        return;
    }

    // pre-build display strings with ANSI as specified
    // RESET cancels Paginator's outer DIM so the tool name renders in BOLD+CYAN
    // TODO: Paginator label extraction strips ESC chars, so name-based selection
    //       falls back to -1 for ANSI-prefixed items — number selection is primary
    vector<string> lines;
    lines.reserve(tools.size());
    for (const SvcDTO::ToolDTO& t : tools) {
        string line;
        if (colorsEnabled()) {
            line += string(Color::RESET) + Color::BOLD + Color::CYAN;
            line += t.name;
            line += string(Color::RESET) + "  " + Color::DIM;
            line += t.short_desc;
            line += Color::RESET;
        } else {
            line = t.name + "  " + t.short_desc;
        }
        lines.push_back(line);
    }

    string    crumb = "tools \u203a " + category;
    Paginator pager(lines, true);

    while (true) {
        pager.render(crumb);

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;
        if (isNext(input)) {
            if (!pager.nextPage())
                cout << "  already on last page.\n";
            continue;
        }
        if (isPrev(input)) {
            if (!pager.prevPage())
                cout << "  already on first page.\n";
            continue;
        }

        int idx = pager.select(input);
        vector<string> clean;
        if (idx == -1) {
            clean.reserve(tools.size());
            for (const auto& t : tools) clean.push_back(t.name);
            idx = matchOption(input, clean);
        }
        if (idx == -1) {
            if (isAmbiguous(input, clean))
                cout << "  " << Strings::get(StringID::TOOLS_AMBIGUOUS) << "\n";
            else
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
            continue;
        }

        showToolDetail(tools[static_cast<size_t>(idx)]);
    }
}

// ==================== CATEGORY BROWSER ====================

static void showCategories() {
    vector<SvcDTO::CategoryDTO> cats = SvcTools::getCategoryList();

    if (cats.empty()) {
        cout << "\n  no categories found.\n\n";
        return;
    }

    // Build display strings: "name — description" (em dash separator)
    vector<string> lines;
    lines.reserve(cats.size());
    for (const auto& c : cats) {
        string line = c.name;
        if (!c.description.empty())
            line += " \u2014 " + c.description;
        lines.push_back(line);
    }

    Paginator      pager(lines, true);
    vector<string> labels = paginatorLabels(lines);

    while (true) {
        pager.render("tools \u203a categories");

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;
        if (isNext(input)) {
            if (!pager.nextPage())
                cout << "  already on last page.\n";
            continue;
        }
        if (isPrev(input)) {
            if (!pager.prevPage())
                cout << "  already on first page.\n";
            continue;
        }

        int idx = pager.select(input);
        if (idx == -1) {
            if (isAmbiguous(input, labels))
                cout << "  ambiguous \u2014 be more specific.\n";
            else
                cout << "  invalid choice \u2014 try again.\n";
            continue;
        }

        showToolsByCategory(cats[static_cast<size_t>(idx)].name);
    }
}

// ==================== SEARCH ====================

static void showSearch() {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a search");
        UI::printDivider();
        cout << "\n"
             << "  enter search query (or 0 to go back):\n";

        string query = readInput("  \u2192 ");
        if (query.empty()) continue;
        if (isQuit(query)) { handleQuit(); return; }
        if (isMenu(query)) throw MenuJump{};
        if (isBack(query)) return;

        vector<SvcDTO::ToolDTO> results = SvcTools::searchTools(query);

        // Build display lines: "name  short_desc"
        vector<string> lines;
        lines.reserve(results.size());
        for (const SvcDTO::ToolDTO& t : results) {
            string line;
            if (colorsEnabled()) {
                line += string(Color::RESET) + Color::BOLD + Color::CYAN;
                line += t.name;
                line += string(Color::RESET) + "  " + Color::DIM;
                line += t.short_desc;
                line += Color::RESET;
            } else {
                line = t.name + "  " + t.short_desc;
            }
            lines.push_back(line);
        }

        if (results.empty()) {
            UI::clearScreen();
            UI::printBanner();
            UI::printBreadcrumb("tools \u203a search \u203a results");
            UI::printDivider();
            cout << "\n"
                 << "  no results for: \"" << query << "\"\n\n";
            UI::printDivider();
            cout << "\n";
            waitForEnter();
            continue;
        }

        string crumb = "tools \u203a search \u203a results (" + to_string(results.size()) + ")";
        Paginator pager(lines, true);

        while (true) {
            pager.render(crumb);

            string input = readInput("  \u2192 ");
            if (input.empty()) continue;
            if (isQuit(input)) { handleQuit(); return; }
            if (isMenu(input)) throw MenuJump{};
            if (isBack(input)) break; // back to search prompt
            if (isNext(input)) {
                if (!pager.nextPage())
                    cout << "  already on last page.\n";
                continue;
            }
            if (isPrev(input)) {
                if (!pager.prevPage())
                    cout << "  already on first page.\n";
                continue;
            }

            int idx = pager.select(input);
            vector<string> clean;
            if (idx == -1) {
                clean.reserve(results.size());
                for (const auto& t : results) clean.push_back(t.name);
                idx = matchOption(input, clean);
            }
            if (idx == -1) {
                if (isAmbiguous(input, clean))
                    cout << "  " << Strings::get(StringID::TOOLS_AMBIGUOUS) << "\n";
                else
                    cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
                continue;
            }

            showToolDetail(results[static_cast<size_t>(idx)]);
        }
    }
}

// ==================== TEMPLATE FILL ====================

static void showTemplateFill(const SvcDTO::ToolDTO& tool,
                              const SvcDTO::TemplateDTO& templ) {
    string target, port;

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a " + tool.category + " \u203a "
                            + tool.name + " \u203a " + templ.template_name);
        UI::printDivider();

        // Template info
        cout << "\n  " << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << templ.template_name
             << (colorsEnabled() ? Color::RESET : "");
        if (!templ.description.empty())
            cout << "  \u2014 " << templ.description;
        cout << "\n\n";

        // ── placeholder prompts ──
        cout << "  enter values for the placeholders below.\n\n";

        // Target (always prompted, required)
        cout << "  target (IP/hostname)";
        if (!target.empty())
            cout << " [" << target << "]";
        cout << ": ";
        string inp = readInput("");
        if (!inp.empty()) {
            if (isQuit(inp)) { handleQuit(); return; }
            if (isMenu(inp)) throw MenuJump{};
            if (isBack(inp)) return;
            target = sanitizeInput(inp);
        }

        // Port (only if template has protocols hint)
        if (!templ.protocols.empty()) {
            cout << "  port (e.g. 80, 1-1000)";
            if (!port.empty())
                cout << " [" << port << "]";
            cout << ": ";
            inp = readInput("");
            if (inp.empty()) {
                // keep existing
            } else if (isQuit(inp)) {
                handleQuit(); return;
            } else if (isMenu(inp)) {
                throw MenuJump{};
            } else if (isBack(inp)) {
                return;
            } else {
                port = sanitizeInput(inp);
            }
        }

        // ── command preview ──
        string cmd = SvcTools::buildCommand(tool, templ, target, port);

        cout << "\n  command preview:\n\n";

        string label = "  $ " + cmd;
        size_t inner = label.size() + 2; // 2 trailing spaces

        string hline;
        hline.reserve(inner * 3);
        for (size_t i = 0; i < inner; ++i) hline += "\u2500";

        cout << "  \u250c" << hline        << "\u2510\n"
             << "  \u2502 " << label << "  " << "\u2502\n"
             << "  \u2514" << hline        << "\u2518\n\n";

        // ── options ──
        cout << (colorsEnabled() ? Color::DIM : "")
             << "  [s] save command   [0] cancel"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        inp = readInput("  \u2192 ");
        if (inp.empty()) continue;
        if (isQuit(inp)) { handleQuit(); return; }

        // Save
        if (inp == "s" || inp == "S") {
            cout << "  note (one-line description): ";
            string note = readInput("");
            if (isQuit(note)) { handleQuit(); return; }
            if (isMenu(note)) throw MenuJump{};
            if (isBack(note)) return;
            if (note.empty()) note = tool.name + " \u2014 " + templ.template_name;

            int id = SvcTools::saveTemplateCommand(tool.id, cmd, note);
            if (id != -1) {
                cout << "\n  " << (colorsEnabled() ? Color::CYAN : "")
                     << "\u2713 command saved (id " << id << ")"
                     << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            } else {
                cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
                     << "! failed to save command"
                     << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            }
            waitForEnter();
            return;
        }

        // Cancel / back
        if (isMenu(inp)) throw MenuJump{};
        if (isBack(inp)) return;

        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
    }
}

// ==================== VULNERABILITY DETAIL ====================

static void showVulnerabilityDetail(const SvcDTO::VulnerabilityDTO& vuln) {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a metasploit \u203a " + vuln.name);
        UI::printDivider();

        cout << "\n  " << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << vuln.name
             << (colorsEnabled() ? Color::RESET : "") << "\n\n";

        cout << "  severity:   " << vuln.severity << "\n"
             << "  access:     " << vuln.access << "\n"
             << "  platform:   " << vuln.platform << "\n"
             << "  service:    " << vuln.service << "\n"
             << "  danger:     " << vuln.danger << "\n"
             << "  discovered: " << vuln.discovered_date
             << " by " << vuln.discoverer << "\n\n";

        if (!vuln.metasploit.empty())
            cout << "  module: " << vuln.metasploit << "\n\n";

        cout << "  " << vuln.description << "\n\n";

        UI::printDivider();
        cout << "\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  [0] " << Strings::get(StringID::TOOLS_BACK)
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;
        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
    }
}

// ==================== MODULE DETAIL ====================

static void showModuleDetail(const SvcDTO::ModuleDTO& mod) {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a recon-ng \u203a " + mod.name);
        UI::printDivider();

        cout << "\n  " << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << mod.name
             << (colorsEnabled() ? Color::RESET : "") << "\n\n";

        cout << "  path:        " << mod.path << "\n"
             << "  platform:    " << mod.platform << "\n"
             << "  type:        " << mod.type << "\n";
        if (!mod.API.empty())
            cout << "  API:         " << mod.API << "\n";
        cout << "  mode:        " << (mod.mode ? "active" : "passive") << "\n"
             << "  loud:        " << (mod.loud ? "yes" : "no") << "\n";
        if (!mod.output.empty())
            cout << "  output:      " << mod.output << "\n";
        cout << "\n  " << mod.description << "\n\n";

        UI::printDivider();
        cout << "\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  [0] " << Strings::get(StringID::TOOLS_BACK)
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;
        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
    }
}

// ==================== MODULES SUB-MENU ====================

static void showModules() {
    vector<SvcDTO::ModuleDTO> modules = SvcTools::getAllModules();
    string filterCol, filterVal;
    bool hasFilter = false;

    auto buildLines = [](const vector<SvcDTO::ModuleDTO>& data)
        -> pair<vector<string>, vector<string>> {
        vector<string> lines, labels;
        lines.reserve(data.size());
        labels.reserve(data.size());
        for (size_t i = 0; i < data.size(); i++) {
            const auto& m = data[i];
            string line = "  ";
            if (colorsEnabled())
                line += string(Color::CYAN) + m.name + Color::RESET;
            else
                line += m.name;
            line += "  " + m.type;
            if (!m.platform.empty()) line += "  " + m.platform;
            lines.push_back(line);

            size_t sp = m.name.find(' ');
            labels.push_back(sp == string::npos ? m.name : m.name.substr(0, sp));
        }
        return {lines, labels};
    };

    auto [lines, labels] = buildLines(modules);
    Paginator pager(lines, true);

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a recon-ng \u203a modules");
        UI::printDivider();

        // ── options ──
        cout << "\n" << (colorsEnabled() ? Color::DIM : "");
        if (hasFilter)
            cout << "  [f] filter: " << filterCol << " = " << filterVal << "\n";
        else
            cout << "  [f] filter\n";
        cout << "  [s] search\n"
             << "  [a] show all"
             << (hasFilter ? " (clear filter)" : "")
             << "\n  [0] back\n"
             << (colorsEnabled() ? Color::RESET : "") << "\n";
        UI::printDivider();

        if (modules.empty()) {
            cout << "\n  no modules found.\n\n";
            UI::printDivider();
            cout << "\n";
            waitForEnter();
            continue;
        }

        pager.render("modules");

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;

        if (isNext(input)) {
            if (!pager.nextPage())
                cout << "  already on last page.\n";
            continue;
        }
        if (isPrev(input)) {
            if (!pager.prevPage())
                cout << "  already on first page.\n";
            continue;
        }

        // Options that change data → rebuild pager
        if (input == "f" || input == "F") {
            cout << "\n  filter by:\n"
                 << "    [1] type\n"
                 << "    [2] platform\n"
                 << "    [0] cancel\n  \u2192 ";
            string fIn = readInput("");
            if (isQuit(fIn)) { handleQuit(); return; }
            if (fIn.empty()) continue;
            if (isMenu(fIn)) throw MenuJump{};
            if (isBack(fIn)) continue;

            string col;
            if (fIn == "1")      col = "type";
            else if (fIn == "2") col = "platform";
            else { cout << "  invalid choice.\n"; waitForEnter(); continue; }

            auto vals = SvcTools::getDistinctModuleValues(col);
            if (vals.empty()) {
                cout << "  no values available.\n";
                waitForEnter();
                continue;
            }
            cout << "\n  select " << col << ":\n";
            for (size_t vi = 0; vi < vals.size(); vi++)
                cout << "    [" << (vi + 1) << "] " << vals[vi] << "\n";
            cout << "  \u2192 ";
            string vIn = readInput("");
            if (isQuit(vIn)) { handleQuit(); return; }
            if (vIn.empty()) continue;
            if (isMenu(vIn)) throw MenuJump{};
            if (isBack(vIn)) continue;
            int vIdx = toNumber(vIn);
            if (vIdx < 1 || vIdx > static_cast<int>(vals.size())) {
                cout << "  invalid choice.\n";
                waitForEnter();
                continue;
            }
            filterCol = col;
            filterVal = vals[static_cast<size_t>(vIdx - 1)];
            hasFilter = true;
            modules = SvcTools::filterModules(filterCol, filterVal);
            tie(lines, labels) = buildLines(modules);
            pager = Paginator(lines, true);
            continue;
        }

        if (input == "s" || input == "S") {
            cout << "\n  search: ";
            string query = readInput("");
            if (isQuit(query)) { handleQuit(); return; }
            if (query.empty()) continue;
            if (isMenu(query)) throw MenuJump{};
            if (isBack(query)) continue;
            modules = SvcTools::searchModules(query);
            hasFilter = false;
            tie(lines, labels) = buildLines(modules);
            pager = Paginator(lines, true);
            continue;
        }

        if (input == "a" || input == "A") {
            modules = SvcTools::getAllModules();
            hasFilter = false;
            tie(lines, labels) = buildLines(modules);
            pager = Paginator(lines, true);
            continue;
        }

        // Select module
        int idx = pager.select(input);
        vector<string> cleanForMatch;
        if (idx == -1) {
            cleanForMatch.reserve(labels.size());
            for (const auto& m : modules) cleanForMatch.push_back(m.name);
            idx = matchOption(input, cleanForMatch);
        }
        if (idx == -1) {
            if (isAmbiguous(input, cleanForMatch.empty() ? labels : cleanForMatch))
                cout << "  ambiguous \u2014 be more specific.\n";
            else
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
            continue;
        }
        showModuleDetail(modules[static_cast<size_t>(idx)]);
    }
}

// ==================== VULNERABILITIES SUB-MENU ====================

static void showVulnerabilities() {
    vector<SvcDTO::VulnerabilityDTO> vulns = SvcTools::getAllVulnerabilities();
    string filterCol, filterVal;
    bool hasFilter = false;

    // Build display lines for current data — helper at function scope
    auto buildLines = [](const vector<SvcDTO::VulnerabilityDTO>& data)
        -> pair<vector<string>, vector<string>> {
        vector<string> lines, labels;
        lines.reserve(data.size());
        labels.reserve(data.size());
        for (size_t i = 0; i < data.size(); i++) {
            const auto& v = data[i];
            string line = "  ";
            if (colorsEnabled())
                line += string(Color::CYAN) + v.name + Color::RESET;
            else
                line += v.name;
            line += "  " + v.severity;
            if (!v.platform.empty()) line += "  " + v.platform;
            if (!v.service.empty())  line += "  " + v.service;
            lines.push_back(line);

            size_t sp = v.name.find(' ');
            labels.push_back(sp == string::npos ? v.name : v.name.substr(0, sp));
        }
        return {lines, labels};
    };

    // Initial build
    auto [lines, labels] = buildLines(vulns);
    Paginator pager(lines, true);

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a metasploit \u203a vulnerabilities");
        UI::printDivider();

        // ── options ──
        cout << "\n"
             << (colorsEnabled() ? Color::DIM : "");
        if (hasFilter)
            cout << "  [f] filter: " << filterCol << " = " << filterVal << "\n";
        else
            cout << "  [f] filter\n";
        cout << "  [s] search\n"
             << "  [a] show all"
             << (hasFilter ? " (clear filter)" : "")
             << "\n  [0] back\n"
             << (colorsEnabled() ? Color::RESET : "") << "\n";
        UI::printDivider();

        if (vulns.empty()) {
            cout << "\n  no vulnerabilities found.\n\n";
            UI::printDivider();
            cout << "\n";
            waitForEnter();
            continue;
        }

        pager.render("vulnerabilities");

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;

        if (isNext(input)) {
            if (!pager.nextPage())
                cout << "  already on last page.\n";
            continue;
        }
        if (isPrev(input)) {
            if (!pager.prevPage())
                cout << "  already on first page.\n";
            continue;
        }

        // Options that change data → rebuild pager
        if (input == "f" || input == "F") {
            cout << "\n  filter by:\n"
                 << "    [1] severity\n"
                 << "    [2] access\n"
                 << "    [3] platform\n"
                 << "    [0] cancel\n  \u2192 ";
            string fIn = readInput("");
            if (isQuit(fIn)) { handleQuit(); return; }
            if (fIn.empty()) continue;
            if (isMenu(fIn)) throw MenuJump{};
            if (isBack(fIn)) continue;

            string col;
            if (fIn == "1")      col = "severity";
            else if (fIn == "2") col = "access";
            else if (fIn == "3") col = "platform";
            else { cout << "  invalid choice.\n"; waitForEnter(); continue; }

            auto vals = SvcTools::getDistinctValues(col);
            if (vals.empty()) {
                cout << "  no values available.\n";
                waitForEnter();
                continue;
            }
            cout << "\n  select " << col << ":\n";
            for (size_t vi = 0; vi < vals.size(); vi++)
                cout << "    [" << (vi + 1) << "] " << vals[vi] << "\n";
            cout << "  \u2192 ";
            string vIn = readInput("");
            if (isQuit(vIn)) { handleQuit(); return; }
            if (vIn.empty()) continue;
            if (isMenu(vIn)) throw MenuJump{};
            if (isBack(vIn)) continue;
            int vIdx = toNumber(vIn);
            if (vIdx < 1 || vIdx > static_cast<int>(vals.size())) {
                cout << "  invalid choice.\n";
                waitForEnter();
                continue;
            }
            filterCol = col;
            filterVal = vals[static_cast<size_t>(vIdx - 1)];
            hasFilter = true;
            vulns = SvcTools::filterVulnerabilities(filterCol, filterVal);
            tie(lines, labels) = buildLines(vulns);
            pager = Paginator(lines, true);
            continue;
        }

        if (input == "s" || input == "S") {
            cout << "\n  search: ";
            string query = readInput("");
            if (isQuit(query)) { handleQuit(); return; }
            if (query.empty()) continue;
            if (isMenu(query)) throw MenuJump{};
            if (isBack(query)) continue;
            vulns = SvcTools::searchVulnerabilities(query);
            hasFilter = false;
            tie(lines, labels) = buildLines(vulns);
            pager = Paginator(lines, true);
            continue;
        }

        if (input == "a" || input == "A") {
            vulns = SvcTools::getAllVulnerabilities();
            hasFilter = false;
            tie(lines, labels) = buildLines(vulns);
            pager = Paginator(lines, true);
            continue;
        }

        // Select vulnerability
        int idx = pager.select(input);
        vector<string> cleanForMatch;
        if (idx == -1) {
            cleanForMatch.reserve(labels.size());
            for (const auto& v : vulns) cleanForMatch.push_back(v.name);
            idx = matchOption(input, cleanForMatch);
        }
        if (idx == -1) {
            if (isAmbiguous(input, cleanForMatch.empty() ? labels : cleanForMatch))
                cout << "  ambiguous \u2014 be more specific.\n";
            else
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
            continue;
        }
        showVulnerabilityDetail(vulns[static_cast<size_t>(idx)]);
    }
}

// ==================== ALL TOOLS ====================

static void showAllTools() {
    vector<SvcDTO::ToolDTO> tools = SvcTools::getAllTools();

    if (tools.empty()) {
        cout << "\n  no tools found.\n\n";
        return;
    }

    // pre-build display strings with ANSI formatting
    vector<string> lines;
    lines.reserve(tools.size());
    for (const SvcDTO::ToolDTO& t : tools) {
        string line;
        if (colorsEnabled()) {
            line += string(Color::RESET) + Color::BOLD + Color::CYAN;
            line += t.name;
            line += string(Color::RESET) + "  " + Color::DIM;
            line += t.short_desc;
            line += Color::RESET;
        } else {
            line = t.name + "  " + t.short_desc;
        }
        lines.push_back(line);
    }

    string    crumb = "tools \u203a all tools";
    Paginator pager(lines, true);

    while (true) {
        pager.render(crumb);

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;
        if (isNext(input)) {
            if (!pager.nextPage())
                cout << "  already on last page.\n";
            continue;
        }
        if (isPrev(input)) {
            if (!pager.prevPage())
                cout << "  already on first page.\n";
            continue;
        }

        int idx = pager.select(input);
        vector<string> clean;
        if (idx == -1) {
            clean.reserve(tools.size());
            for (const auto& t : tools) clean.push_back(t.name);
            idx = matchOption(input, clean);
        }
        if (idx == -1) {
            if (isAmbiguous(input, clean))
                cout << "  " << Strings::get(StringID::TOOLS_AMBIGUOUS) << "\n";
            else
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
            continue;
        }

        showToolDetail(tools[static_cast<size_t>(idx)]);
    }
}

// ==================== SAVED TEMPLATES ====================

static void showMyTemplates() {
    while (true) {
        auto all = SvcSavedTemplates::getAllTemplates();
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a my templates");
        UI::printDivider();

        if (all.empty()) {
            cout << "\n  " << Strings::get(StringID::SAVED_TEMPLATES_EMPTY) << "\n\n";
            cout << (colorsEnabled() ? Color::DIM : "")
                 << "  [a] " << Strings::get(StringID::SAVED_TEMPLATES_ADD) << "\n"
                 << "  [0] " << Strings::get(StringID::TOOLS_BACK) << "\n"
                 << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            string input = readInput("  \u2192 (m=menu) ");
            if (input.empty()) continue;
            if (isQuit(input)) { handleQuit(); return; }
            if (isMenu(input)) throw MenuJump{};
            if (isBack(input)) return;
            if (input == "a" || input == "A") { addNewTemplate(); continue; }
            cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
            waitForEnter();
            continue;
        }

        // Build display lines
        vector<string> lines;
        for (const auto& t : all) {
            string line = t.name;
            if (!t.description.empty())
                line += "  \u2014 " + t.description;
            line += "  (" + t.tool_name + ")";
            lines.push_back(line);
        }
        Paginator pager(lines, true);
        string crumb = "tools \u203a my templates";

        while (true) {
            pager.render(crumb);
            cout << (colorsEnabled() ? Color::DIM : "")
                 << "  [a] " << Strings::get(StringID::SAVED_TEMPLATES_ADD) << "   "
                 << "[d] delete   [0] " << Strings::get(StringID::TOOLS_BACK)
                 << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            string input = readInput("  \u2192 (m=menu) ");
            if (input.empty()) continue;
            if (isQuit(input)) { handleQuit(); return; }
            if (isMenu(input)) throw MenuJump{};
            if (isBack(input)) return;
            if (isNext(input)) { if (!pager.nextPage()) cout << "  already on last page.\n"; continue; }
            if (isPrev(input)) { if (!pager.prevPage()) cout << "  already on first page.\n"; continue; }

            if (input == "a" || input == "A") { addNewTemplate(); break; }

            if (input == "d" || input == "D") {
                cout << "\n  " << Strings::get(StringID::SAVED_TEMPLATES_CONFIRM_DELETE) << " ";
                string confirm = readInput("");
                if (isQuit(confirm)) { handleQuit(); return; }
                if (isMenu(confirm)) throw MenuJump{};
                if (isBack(confirm)) continue;
                if (confirm == "y" || confirm == "Y") {
                    cout << "  enter number: ";
                    string delNum = readInput("");
                    if (isQuit(delNum)) { handleQuit(); return; }
                    if (isMenu(delNum)) throw MenuJump{};
                    if (isBack(delNum)) continue;
                    int di = toNumber(delNum);
                    if (di >= 1 && di <= static_cast<int>(all.size())) {
                        if (SvcSavedTemplates::deleteTemplate(all[di - 1].id))
                            cout << "  " << Strings::get(StringID::SAVED_TEMPLATES_DELETED) << "\n";
                        else
                            cout << "  ! failed to delete.\n";
                    } else {
                        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
                    }
                }
                waitForEnter();
                break;
            }

            int idx = pager.select(input);
            if (idx == -1) {
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
                continue;
            }
            useSavedTemplate(all[idx]);
        }
    }
}

// ==================== ADD NEW TEMPLATE ====================

static void addNewTemplate() {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a add template");
        UI::printDivider();

        cout << "\n  select method:\n\n"
             << "  [1] " << Strings::get(StringID::SAVED_TEMPLATES_BROWSE) << "\n"
             << "  [2] " << Strings::get(StringID::SAVED_TEMPLATES_SEARCH_TOOL) << "\n"
             << "  [3] " << Strings::get(StringID::SAVED_TEMPLATES_MAKE_OWN) << "\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  [0] " << Strings::get(StringID::TOOLS_BACK) << "\n"
             << (colorsEnabled() ? Color::RESET : "") << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;
        int num = toNumber(input);
        if (num < 1 || num > 3) {
            cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
            waitForEnter();
            continue;
        }

        // --- Select a tool first (common to all three options) ---
        SvcDTO::ToolDTO selectedTool;
        if (num == 1) {
            // Browse by Category
            auto catList = SvcTools::getCategoryList();
            if (catList.empty()) {
                cout << "  no categories available.\n"; waitForEnter(); continue;
            }
            while (true) {
                UI::clearScreen();
                UI::printBanner();
                UI::printBreadcrumb("tools \u203a add template \u203a browse");
                UI::printDivider();
                cout << "\n  select category:\n\n";
                for (size_t i = 0; i < catList.size(); i++)
                    cout << "  [" << (i+1) << "] " << catList[i].name << "\n";
                cout << (colorsEnabled() ? Color::DIM : "")
                     << "  [0] back\n"
                     << (colorsEnabled() ? Color::RESET : "") << "\n";
                string ci = readInput("  \u2192 ");
                if (isQuit(ci)) { handleQuit(); return; }
                if (isMenu(ci)) throw MenuJump{};
                if (isBack(ci)) break;
                int ciNum = toNumber(ci);
                if (ciNum < 1 || ciNum > static_cast<int>(catList.size())) {
                    cout << "  invalid.\n"; waitForEnter(); continue;
                }
                auto tools = SvcTools::getToolsByCategory(catList[ciNum-1].name);
                if (tools.empty()) { cout << "  no tools.\n"; waitForEnter(); continue; }
                while (true) {
                    UI::clearScreen();
                    UI::printBanner();
                    UI::printBreadcrumb("tools \u203a add template \u203a browse \u203a " + catList[ciNum-1].name);
                    UI::printDivider();
                    cout << "\n  select tool:\n\n";
                    for (size_t i = 0; i < tools.size(); i++)
                        cout << "  [" << (i+1) << "] " << tools[i].name << "\n";
                    cout << (colorsEnabled() ? Color::DIM : "")
                         << "  [0] back\n"
                         << (colorsEnabled() ? Color::RESET : "") << "\n";
                    string ti = readInput("  \u2192 ");
                    if (isQuit(ti)) { handleQuit(); return; }
                    if (isMenu(ti)) throw MenuJump{};
                    if (isBack(ti)) break;
                    int tiNum = toNumber(ti);
                    if (tiNum < 1 || tiNum > static_cast<int>(tools.size())) {
                        cout << "  invalid.\n"; waitForEnter(); continue;
                    }
                    selectedTool = tools[tiNum-1];
                    break;
                }
                if (selectedTool.id != 0) break;
            }
        } else if (num == 2) {
            // Search Tool
            while (true) {
                UI::clearScreen();
                UI::printBanner();
                UI::printBreadcrumb("tools \u203a add template \u203a search");
                UI::printDivider();
                cout << "\n  search: ";
                string query = readInput("");
                if (isQuit(query)) { handleQuit(); return; }
                if (isMenu(query)) throw MenuJump{};
                if (isBack(query)) break;
                auto results = SvcTools::searchTools(query);
                if (results.empty()) {
                    cout << "  no tools found.\n"; waitForEnter(); continue;
                }
                while (true) {
                    UI::clearScreen();
                    UI::printBanner();
                    UI::printBreadcrumb("tools \u203a add template \u203a search results");
                    UI::printDivider();
                    cout << "\n  results for \"" << query << "\":\n\n";
                    for (size_t i = 0; i < results.size(); i++)
                        cout << "  [" << (i+1) << "] " << results[i].name << "\n";
                    cout << (colorsEnabled() ? Color::DIM : "")
                         << "  [0] back\n"
                         << (colorsEnabled() ? Color::RESET : "") << "\n";
                    string ri = readInput("  \u2192 ");
                    if (isQuit(ri)) { handleQuit(); return; }
                    if (isMenu(ri)) throw MenuJump{};
                    if (isBack(ri)) break;
                    int riNum = toNumber(ri);
                    if (riNum < 1 || riNum > static_cast<int>(results.size())) {
                        cout << "  invalid.\n"; waitForEnter(); continue;
                    }
                    selectedTool = results[riNum - 1];
                    break;
                }
                if (selectedTool.id != 0) break;
            }
        } else if (num == 3) {
            // Make Own Template — first select a tool then create content
            auto catList = SvcTools::getCategoryList();
            if (catList.empty()) {
                cout << "  no categories available.\n"; waitForEnter(); continue;
            }
            while (true) {
                UI::clearScreen();
                UI::printBanner();
                UI::printBreadcrumb("tools \u203a add template \u203a make own");
                UI::printDivider();
                cout << "\n  select a tool for this template:\n\n";
                for (size_t i = 0; i < catList.size(); i++)
                    cout << "  [" << (i+1) << "] " << catList[i].name << "\n";
                cout << (colorsEnabled() ? Color::DIM : "")
                     << "  [0] back\n"
                     << (colorsEnabled() ? Color::RESET : "") << "\n";
                string ci = readInput("  \u2192 ");
                if (isQuit(ci)) { handleQuit(); return; }
                if (isMenu(ci)) throw MenuJump{};
                if (isBack(ci)) break;
                int ciNum = toNumber(ci);
                if (ciNum < 1 || ciNum > static_cast<int>(catList.size())) {
                    cout << "  invalid.\n"; waitForEnter(); continue;
                }
                auto tools = SvcTools::getToolsByCategory(catList[ciNum-1].name);
                if (tools.empty()) { cout << "  no tools.\n"; waitForEnter(); continue; }
                while (true) {
                    UI::clearScreen();
                    UI::printBanner();
                    UI::printBreadcrumb("tools \u203a add template \u203a make own \u203a " + catList[ciNum-1].name);
                    UI::printDivider();
                    cout << "\n  select tool:\n\n";
                    for (size_t i = 0; i < tools.size(); i++)
                        cout << "  [" << (i+1) << "] " << tools[i].name << "\n";
                    cout << (colorsEnabled() ? Color::DIM : "")
                         << "  [0] back\n"
                         << (colorsEnabled() ? Color::RESET : "") << "\n";
                    string ti = readInput("  \u2192 ");
                    if (isQuit(ti)) { handleQuit(); return; }
                    if (isMenu(ti)) throw MenuJump{};
                    if (isBack(ti)) break;
                    int tiNum = toNumber(ti);
                    if (tiNum < 1 || tiNum > static_cast<int>(tools.size())) {
                        cout << "  invalid.\n"; waitForEnter(); continue;
                    }
                    selectedTool = tools[tiNum-1];
                    break;
                }
                if (selectedTool.id != 0) break;
            }
        }

        if (selectedTool.id == 0) {
            cout << "  no tool selected.\n"; waitForEnter(); continue;
        }

        // Now handle the actual template creation based on the path
        if (num == 1 || num == 2) {
            // Browse/Search: user selects a DB template to save
            auto templates = SvcTools::getTemplatesByToolId(selectedTool.id);
            if (templates.empty()) {
                cout << "  no templates for this tool.\n"; waitForEnter(); continue;
            }
            UI::clearScreen();
            UI::printBanner();
            UI::printBreadcrumb("tools \u203a add template \u203a " + selectedTool.name);
            UI::printDivider();
            cout << "\n  select a template to save:\n\n";
            for (size_t i = 0; i < templates.size(); i++)
                cout << "  [" << (i+1) << "] " << templates[i].template_name
                     << (!templates[i].description.empty() ? "  \u2014 " + templates[i].description : "")
                     << "\n";
            cout << (colorsEnabled() ? Color::DIM : "")
                 << "  [0] cancel\n"
                 << (colorsEnabled() ? Color::RESET : "") << "\n";
            string pi = readInput("  \u2192 ");
            if (isQuit(pi)) { handleQuit(); return; }
            if (isMenu(pi)) throw MenuJump{};
            if (isBack(pi)) { continue; }
            int piNum = toNumber(pi);
            if (piNum < 1 || piNum > static_cast<int>(templates.size())) {
                cout << "  invalid.\n"; waitForEnter(); continue;
            }
            const auto& tmpl = templates[piNum - 1];

            // Build command to show preview
            string previewCmd = SvcTools::buildCommand(selectedTool, tmpl, "{{target}}", "{{port}}");

            cout << "\n  template: " << tmpl.template_name << "\n"
                 << "  command:  " << previewCmd << "\n\n";
            cout << "  " << Strings::get(StringID::SAVED_TEMPLATES_NAME_PROMPT)
                 << " [" << tmpl.template_name << "]: ";
            string name = readInput("");
            if (isQuit(name)) { handleQuit(); return; }
            if (isMenu(name)) throw MenuJump{};
            if (name.empty()) name = tmpl.template_name;

            cout << "  " << Strings::get(StringID::SAVED_TEMPLATES_DESC_PROMPT)
                 << " [" << tmpl.description << "]: ";
            string desc = readInput("");
            if (isQuit(desc)) { handleQuit(); return; }
            if (isMenu(desc)) throw MenuJump{};
            if (desc.empty()) desc = tmpl.description;

            int id = SvcSavedTemplates::saveTemplate(selectedTool.id, name, previewCmd, desc);
            if (id != -1) {
                cout << "\n  " << (colorsEnabled() ? Color::GREEN : "")
                     << "\u2713 template saved (id " << id << ")"
                     << (colorsEnabled() ? Color::RESET : "") << "\n";
            } else {
                cout << "\n  ! failed to save template.\n";
            }
            waitForEnter();
            return;
        } else {
            // Make Own Template: user enters custom content
            cout << "\n  " << Strings::get(StringID::SAVED_TEMPLATES_NAME_PROMPT) << ": ";
            string tName = readInput("");
            if (isQuit(tName)) { handleQuit(); return; }
            if (isMenu(tName)) throw MenuJump{};
            if (isBack(tName)) continue;
            if (tName.empty()) {
                cout << "  name cannot be empty.\n"; waitForEnter(); continue;
            }

            cout << "\n  " << Strings::get(StringID::SAVED_TEMPLATES_CONTENT_PROMPT) << ":\n"
                 << "  (use {{target}} and {{port}} as placeholders)\n"
                 << "  \u2192 ";
            string content = readInput("");
            if (isQuit(content)) { handleQuit(); return; }
            if (isMenu(content)) throw MenuJump{};
            if (isBack(content)) continue;
            if (content.empty()) {
                cout << "  content cannot be empty.\n"; waitForEnter(); continue;
            }

            cout << "\n  " << Strings::get(StringID::SAVED_TEMPLATES_DESC_PROMPT) << ": ";
            string desc = readInput("");
            if (isQuit(desc)) { handleQuit(); return; }
            if (isMenu(desc)) throw MenuJump{};
            if (isBack(desc)) desc = "";

            cout << "\n  preview:  $ " << content << "\n\n";
            cout << "  save this template? (y/n): ";
            string confirm = readInput("");
            if (isQuit(confirm)) { handleQuit(); return; }
            if (isMenu(confirm)) throw MenuJump{};
            if (isBack(confirm)) continue;
            if (confirm == "y" || confirm == "Y") {
                int id = SvcSavedTemplates::saveTemplate(selectedTool.id, tName, content, desc);
                if (id != -1) {
                    cout << "  " << (colorsEnabled() ? Color::GREEN : "")
                         << "\u2713 custom template saved (id " << id << ")"
                         << (colorsEnabled() ? Color::RESET : "") << "\n";
                } else {
                    cout << "  ! failed to save template.\n";
                }
                waitForEnter();
            }
            return;
        }
    }
}

// ==================== USE SAVED TEMPLATE ====================

static void useSavedTemplate(const SvcDTO::SavedTemplateDTO& st) {
    string target, port;
    string content = st.content;

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a " + st.tool_name + " \u203a " + st.name + " (saved)");
        UI::printDivider();

        cout << "\n  " << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << st.name << " (saved)"
             << (colorsEnabled() ? Color::RESET : "");
        if (!st.description.empty())
            cout << "  \u2014 " << st.description;
        cout << "\n\n  enter values for the placeholders below.\n\n";

        // Target (always prompted)
        cout << "  target (IP/hostname)";
        if (!target.empty()) cout << " [" << target << "]";
        cout << ": ";
        string inp = readInput("");
        if (isQuit(inp)) { handleQuit(); return; }
        if (isMenu(inp)) throw MenuJump{};
        if (isBack(inp)) return;
        if (!inp.empty()) target = sanitizeInput(inp);

        // Port (prompted if content has {{port}} or {{PORT}})
        if (content.find("{{port}}") != string::npos || content.find("{{PORT}}") != string::npos) {
            cout << "  port (e.g. 80, 1-1000)";
            if (!port.empty()) cout << " [" << port << "]";
            cout << ": ";
            inp = readInput("");
            if (isQuit(inp)) { handleQuit(); return; }
            if (isMenu(inp)) throw MenuJump{};
            if (isBack(inp)) return;
            if (!inp.empty()) port = sanitizeInput(inp);
        }

        // Build command by replacing placeholders
        string cmd = content;
        size_t pos;
        while ((pos = cmd.find("{{target}}")) != string::npos)
            cmd.replace(pos, 10, target.empty() ? "TARGET" : target);
        while ((pos = cmd.find("{{port}}")) != string::npos)
            cmd.replace(pos, 8, port.empty() ? "PORT" : port);
        while ((pos = cmd.find("{{TARGET}}")) != string::npos)
            cmd.replace(pos, 10, target.empty() ? "TARGET" : target);
        while ((pos = cmd.find("{{PORT}}")) != string::npos)
            cmd.replace(pos, 8, port.empty() ? "PORT" : port);

        // Preview
        cout << "\n  command preview:\n\n";
        string label = "  $ " + cmd;
        size_t inner = label.size() + 2;
        string hline;
        hline.reserve(inner * 3);
        for (size_t i = 0; i < inner; ++i) hline += "\u2500";
        cout << "  \u250c" << hline << "\u2510\n"
             << "  \u2502 " << label << "  " << "\u2502\n"
             << "  \u2514" << hline << "\u2518\n\n";

        cout << (colorsEnabled() ? Color::DIM : "")
             << "  [s] save command   [0] cancel"
             << (colorsEnabled() ? Color::RESET : "") << "\n\n";

        inp = readInput("  \u2192 ");
        if (inp.empty()) continue;
        if (isQuit(inp)) { handleQuit(); return; }
        if (isMenu(inp)) throw MenuJump{};
        if (isBack(inp)) return;

        if (inp == "s" || inp == "S") {
            cout << "  note (one-line description): ";
            string note = readInput("");
            if (isQuit(note)) { handleQuit(); return; }
            if (isMenu(note)) throw MenuJump{};
            if (isBack(note)) return;
            if (note.empty()) note = st.tool_name + " \u2014 " + st.name;

            int id = SvcTools::saveTemplateCommand(st.tool_id, cmd, note);
            if (id != -1) {
                cout << "\n  " << (colorsEnabled() ? Color::CYAN : "")
                     << "\u2713 command saved (id " << id << ")"
                     << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            } else {
                cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
                     << "! failed to save command"
                     << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            }
            waitForEnter();
            return;
        }
    }
}

// ==================== TOOLS ENTRY ====================

void UITools::show() {
    const vector<string> opts = {
        Strings::get(StringID::TOOLS_VIEW_ALL),
        Strings::get(StringID::TOOLS_BROWSE_CATEGORY),
        Strings::get(StringID::TOOLS_SEARCH),
        Strings::get(StringID::SAVED_TEMPLATES_LIST)
    };

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools");
        UI::printDivider();

        cout << "\n"
             << "  \u251C\u2500 \u2605  " << Strings::get(StringID::TOOLS_VIEW_ALL) << "        [1]\n"
             << "  \u251C\u2500 \u25C9  " << Strings::get(StringID::TOOLS_BROWSE_CATEGORY) << "   [2]\n"
             << "  \u251C\u2500 \u2315  " << Strings::get(StringID::TOOLS_SEARCH) << "               [3]\n"
             << "  \u251C\u2500 \u2630  " << Strings::get(StringID::SAVED_TEMPLATES_LIST) << "       [4]\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2514\u2500 \u2190  " << Strings::get(StringID::TOOLS_BACK) << "                 [0]"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        UI::printDivider();
        cout << "\n";

        string input = readInput("  " + Strings::get(StringID::TOOLS_PROMPT) + " (m=menu) ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isMenu(input)) throw MenuJump{};
        if (isBack(input)) return;

        int idx = matchOption(input, opts);

        if (idx == 0) {
            showAllTools();
        } else if (idx == 1) {
            showCategories();
        } else if (idx == 2) {
            showSearch();
        } else if (idx == 3) {
            showMyTemplates();
        } else {
            if (isAmbiguous(input, opts))
                cout << "  " << Strings::get(StringID::TOOLS_AMBIGUOUS) << "\n";
            else
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        }
    }
}