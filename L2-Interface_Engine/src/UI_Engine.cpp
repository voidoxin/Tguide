/*
 *  tguide — UI_Engine.cpp
 *  written by voidoxin
 */

#include <iostream>
#include <string>
#include <vector>

#include "../includes/UI_colors.h"
#include "../includes/UI_Engine.h"
#ifdef TGUIDE_ENABLE_GENERATOR
#include "../includes/UI_generator.h"
#endif
#include "../includes/UI_input.h"
#include "../includes/UI_savedCommands.h"
#include "../includes/UI_savedScripts.h"
#include "../includes/UI_settings.h"
#include "../includes/UI_tools.h"
#include "../includes/UI_utils.h"
#include "../../L0-core/include/config_manager.h"

using namespace std;

// ── internal: print a single menu item ────────────────────────────────────
static void printItem(int index, const UIEngine::MenuItem& item, bool isLast) {
    string connector = isLast ? "└─" : "├─";

    cout << Color::DIM   << "  " << connector << " " << Color::RESET
         << Color::CYAN  << item.icon << "  "        << Color::RESET
         << Color::BOLD  << item.label                << Color::RESET;

    if (!item.hint.empty()) {
        cout << Color::DIM << "  —  " << item.hint << Color::RESET;
    }

    cout << "  "
         << Color::DIM << "[" << Color::RESET
         << Color::YELLOW << index << Color::RESET
         << Color::DIM << "]" << Color::RESET
         << "\n";
}

// ── internal: print invalid input warning ─────────────────────────────────
// intentionally NOT using UI_errors() — input validation, not a system error
static void printInvalidInput() {
    cout << (colorsEnabled() ? Color::YELLOW : "")
         << "\n  invalid choice — try again.\n\n"
         << (colorsEnabled() ? Color::RESET : "");
}

// ── renderMenu ─────────────────────────────────────────────────────────────
int UIEngine::renderMenu(
    const string&           section,
    const vector<MenuItem>& items)
{
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb(section);

        for (size_t i = 0; i < items.size(); i++) {
            bool isLast = (i == items.size() - 1);
            printItem(static_cast<int>(i + 1), items[i], isLast);
        }

        cout << "\n"
             << Color::DIM << "  ─── " << Color::RESET
             << Color::BOLD << "choose" << Color::RESET
             << Color::DIM << " ──────────────────────\n" << Color::RESET;

        string input = readInput("  → ");
        if (isQuit(input)) { handleQuit(); return 0; }   // 0 = quit sentinel

        int choice = toNumber(input);

        if (choice < 1 || choice > static_cast<int>(items.size())) {
            printInvalidInput();
            continue;
        }

        // ── show what was selected before running the action ──────────────
        UI::clearScreen();
        UI::printBanner();

        cout << Color::DIM   << "  last: " << Color::RESET
             << Color::CYAN  << items[choice - 1].icon << "  " << Color::RESET
             << Color::BOLD  << items[choice - 1].label << Color::RESET
             << "\n\n";

        UI::printDivider();
        cout << "\n";

        // run the action if set
        if (items[choice - 1].action)
            items[choice - 1].action();

        return choice;
    }
}

// ── start — main UI loop ───────────────────────────────────────────────────
void UIEngine::start(ConfigManager& cfg) {

    while (true) {

        vector<MenuItem> mainMenu = {
            { "⊞", "Tools",            "flags, templates, usage",       []() { UITools::show();           } },
#ifdef TGUIDE_ENABLE_GENERATOR
            { "◎", "Script Generator", "build commands interactively",   []() { UIGenerator::show();       } },
#endif
            { "◈", "Saved Commands",   "your personal command library",  []() { UISavedCommands::show();   } },
            { "▦", "Saved Scripts",    "your generated scripts",         []() { UISavedScripts::show();    } },
            { "⊙", "Settings",         "configure tguide behavior",      [&cfg]() { UISettings::show(cfg); } },
            { "✕", "Exit",             "",                               nullptr                            },
        };

        int choice = renderMenu("main menu", mainMenu);

        if (choice == 0) {
            // 0 = Ctrl+C quit sentinel — handleQuit() already printed goodbye
            break;
        }

        if (choice == static_cast<int>(mainMenu.size())) {
            // last item always = exit
            UI::clearScreen();
            cout << Color::CYAN << Color::BOLD
                 << "\n  goodbye.\n\n"
                 << Color::RESET;
            break;
        }
    }
}
