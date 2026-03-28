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
#include "../../L0-core/include/DatabaseManager.h"
#include "../../L0-core/include/path_resolver.h"
#include "../../L1-services/includes/svc_tools.h" #include <iostream>
#include <string>                                 #include <vector>

using namespace std;

// ── forward declarations ────────────────────────────────────────────────────
static void showToolDetail(const Tool& tool);
static void showToolsByCategory(ToolD& db, const string& category);
static void showCategories(ToolD& db);
static void showSearch(ToolD& db);

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

// TODO: implement full detail screen in Task 4
static void showToolDetail(const Tool& tool) {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("tools \u203a " + tool.name);
    UI::printDivider();
    cout << "\n"
         << (colorsEnabled() ? Color::DIM : "")
         << "  tool detail \u2014 coming in Task 4"
         << (colorsEnabled() ? Color::RESET : "")
         << "\n\n";
    UI::printDivider();
    cout << "\n";
    pause();
}

// ==================== TOOL LIST ====================

static void showToolsByCategory(ToolD& db, const string& category) {
    vector<Tool> tools = SvcTools::getToolsByCategory(db, category);

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
    for (const Tool& t : tools) {
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
        if (idx == -1) {
            // name-based selection degrades for ANSI items; just report invalid
            cout << "  invalid choice \u2014 try again.\n";
            continue;
        }

        showToolDetail(tools[static_cast<size_t>(idx)]);
    }
}

// ==================== CATEGORY BROWSER ====================

static void showCategories(ToolD& db) {
    vector<string> cats = SvcTools::getCategories(db);

    if (cats.empty()) {
        cout << "\n  no categories found.\n\n";
        return;
    }

    Paginator      pager(cats, true);
    vector<string> labels = paginatorLabels(cats);

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

        showToolsByCategory(db, cats[static_cast<size_t>(idx)]);
    }
}

// ==================== SEARCH ====================

static void showSearch(ToolD& db) {
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
        pause();
        // loop returns to search prompt
    }
}

// ==================== TOOLS ENTRY ====================

void UITools::show() {
    // DB owned locally — temporary until a session context object exists
    ToolD db(PathResolver::dbFile().string());

    const vector<string> opts = {"Browse by Category", "Search", "Filter"};

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("tools");
        UI::printDivider();

        cout << "\n"
             << "  \u251C\u2500 \u25C9  Browse by Category   [1]\n"
             << "  \u251C\u2500 \u2315  Search               [2]\n"
             << "  \u251C\u2500 \u22DF  Filter               [3]\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2514\u2500 \u2190  Back                 [0]"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        UI::printDivider();
        cout << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        int idx = matchOption(input, opts);

        if (idx == 0) {
            showCategories(db);
        } else if (idx == 1) {
            showSearch(db);
        } else if (idx == 2) {
            UI::clearScreen();
            UI::printBanner();
            UI::printBreadcrumb("tools \u203a filter");
            UI::printDivider();
            cout << "\n"
                 << (colorsEnabled() ? Color::DIM : "")
                 << "  coming soon \u2014 not yet implemented"
                 << (colorsEnabled() ? Color::RESET : "")
                 << "\n\n";
            UI::printDivider();
            cout << "\n";
            pause();
        } else {
            if (isAmbiguous(input, opts))
                cout << "  ambiguous \u2014 be more specific.\n";
            else
                cout << "  invalid choice \u2014 try again.\n";
        }
    }
}