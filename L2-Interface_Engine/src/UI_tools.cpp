/*
 *  tguide — UI_tools.cpp
 *  tools entry screen, category browser, tool list per category
 *
 *  written by voidoxin
 */

#include "../includes/UI_tools.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include "../includes/UI_paginator.h"
#include "../../L1-services/includes/svc_tools.h"
#include "../../L1-services/includes/svc_strings.h"
#include "../../L1-services/includes/svc_generator.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// ── forward declarations ────────────────────────────────────────────────────
static void showToolDetail(const SvcDTO::ToolDTO& tool);
static void showToolsByCategory(const string& category);
static void showCategories();
static void showSearch();
static void showTemplateFill(const SvcDTO::ToolDTO& tool,
                              const SvcDTO::TemplateDTO& templ);
static void showVulnerabilities();

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

static bool isMetasploit(const SvcDTO::ToolDTO& tool) {
    string lo = tool.name;
    transform(lo.begin(), lo.end(), lo.begin(),
              ::tolower);
    return lo == "metasploit";
}


// ==================== TOOL DETAIL (STUB) ====================

static void showToolDetail(const SvcDTO::ToolDTO& tool) {
    vector<SvcDTO::ToolFlagDTO> flags    = SvcTools::getFlagsByToolId(tool.id);
    vector<SvcDTO::TemplateDTO> templates = SvcTools::getTemplatesByToolId(tool.id);

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

        // ── recon-ng placeholder (future step) ──
        // TODO: STEP-16 — modules sub-menu for recon-ng

        UI::printDivider();
        cout << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        // Check if input is a template number (1..N)
        int num = toNumber(input);
        if (num >= 1 && num <= static_cast<int>(templates.size())) {
            showTemplateFill(tool, templates[static_cast<size_t>(num - 1)]);
            continue;
        }

        // Metasploit vulnerabilities
        if (isMetasploit(tool) && (input == "v" || input == "V")) {
            showVulnerabilities();
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

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        // results screen — algorithm is a future task
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools \u203a search \u203a results");
        UI::printDivider();
        cout << "\n"
             << "  searching...\n\n";

        string content = "  no results for: \"" + input + "\"";
        size_t inner   = content.size() + 2; // 2 trailing spaces of padding

        string hline;
        hline.reserve(inner * 3);
        for (size_t i = 0; i < inner; ++i) hline += "\u2500";

        cout << "  \u250c" << hline        << "\u2510\n"
             << "  \u2502" << content << "  " << "\u2502\n"
             << "  \u2514" << hline        << "\u2518\n\n"
             << "  search engine not yet implemented.\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  press enter to search again or 0 to go back."
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";
        UI::printDivider();
        cout << "\n";
        waitForEnter();
        // loop returns to search prompt
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
            if (isQuit(inp) || isBack(inp)) return;
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
            } else if (isQuit(inp) || isBack(inp)) {
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
        if (isBack(input)) return;
        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
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
            if (fIn.empty() || isQuit(fIn) || isBack(fIn)) continue;

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
            if (vIn.empty() || isQuit(vIn) || isBack(vIn)) continue;
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
            if (query.empty() || isQuit(query) || isBack(query)) continue;
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

// ==================== TOOLS ENTRY ====================

void UITools::show() {
    const vector<string> opts = {
        Strings::get(StringID::TOOLS_BROWSE_CATEGORY),
        Strings::get(StringID::TOOLS_SEARCH),
        Strings::get(StringID::TOOLS_FILTER)
    };

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools");
        UI::printDivider();

        cout << "\n"
             << "  \u251C\u2500 \u25C9  " << Strings::get(StringID::TOOLS_BROWSE_CATEGORY) << "   [1]\n"
             << "  \u251C\u2500 \u2315  " << Strings::get(StringID::TOOLS_SEARCH) << "               [2]\n"
             << "  \u251C\u2500 \u22DF  " << Strings::get(StringID::TOOLS_FILTER) << "               [3]\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2514\u2500 \u2190  " << Strings::get(StringID::TOOLS_BACK) << "                 [0]"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        UI::printDivider();
        cout << "\n";

        string input = readInput("  " + Strings::get(StringID::TOOLS_PROMPT) + " ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        int idx = matchOption(input, opts);

        if (idx == 0) {
            showCategories();
        } else if (idx == 1) {
            showSearch();
        } else if (idx == 2) {
            UI::clearScreen();
            UI::printBanner();
            UI::printBreadcrumb("tools \u203a filter");
            UI::printDivider();
            cout << "\n"
                 << (colorsEnabled() ? Color::DIM : "")
                 << Strings::get(StringID::TOOLS_COMING_SOON)
                 << (colorsEnabled() ? Color::RESET : "")
                 << "\n\n";
            UI::printDivider();
            cout << "\n";
            waitForEnter();
        } else {
            if (isAmbiguous(input, opts))
                cout << "  " << Strings::get(StringID::TOOLS_AMBIGUOUS) << "\n";
            else
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        }
    }
}