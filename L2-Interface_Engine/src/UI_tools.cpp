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
#include <iostream>
#include <string>
#include <vector>

using namespace std;

// ── forward declarations ────────────────────────────────────────────────────
static void showToolDetail(const SvcDTO::ToolDTO& tool);
static void showToolsByCategory(const string& category);
static void showCategories();
static void showSearch();

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
            for (const auto& t : templates) {
                cout << "    " << (colorsEnabled() ? Color::CYAN : "") << t.template_name
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

        // ── metasploit / recon-ng placeholder (future steps) ──
        // TODO: STEP-15 — vulnerabilities sub-menu for metasploit
        // TODO: STEP-16 — modules sub-menu for recon-ng

        UI::printDivider();
        cout << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        // If user typed just the tool name or a prefix, interpret as "back"
        // (normal behavior: any unrecognized input → invalid, loop)
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