/*
 *  tguide — UI_disclaimer.cpp
 *  written by voidoxin
 */

#include "../includes/UI_disclaimer.h"
#include "../../L0-core/include/config_manager.h"
#include "../includes/UI_input.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_utils.h"
#include <iostream>
#include <string>
#include <algorithm>

using namespace std;

// ==================== UIDisclaimer ====================

bool UIDisclaimer::show(ConfigManager& cfg) {
    while (true) {
        UI::clearScreen();

        // ── header ────────────────────────────────────────────────────────
        cout << (colorsEnabled() ? Color::CYAN : "")
             << (colorsEnabled() ? Color::BOLD : "");
        cout << "  \u2554\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2557\n";
        cout << "  \u2551               TGUIDE  \u2014  LEGAL DISCLAIMER"
                "                   \u2551\n";
        cout << "  \u255a\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550"
                "\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u2550\u255d\n";
        cout << (colorsEnabled() ? Color::RESET : "");

        // ── body ──────────────────────────────────────────────────────────
        cout << "\n"
             << "  This tool is intended solely for authorized security testing,\n"
             << "  educational purposes, and lawful penetration testing activities.\n"
             << "\n"
             << "  By using tguide, you confirm that:\n"
             << "\n"
             << "    \u2192  You have explicit written permission to test the target systems.\n"
             << "    \u2192  You are not using this tool for unauthorized access, surveillance,\n"
             << "       or any activity that violates local, national, or international law.\n"
             << "    \u2192  You accept full responsibility for any actions performed\n"
             << "       using this tool.\n"
             << "\n"
             << "  The author (voidoxin) provides this software \"as is\" without warranty\n"
             << "  of any kind and bears no liability for damages, legal consequences,\n"
             << "  or misuse arising from the use of this tool.\n"
             << "\n"
             << "  Unauthorized use of this tool against systems you do not own\n"
             << "  or have explicit permission to test is illegal and punishable by law.\n"
             << "\n";

        // ── divider ───────────────────────────────────────────────────────
        cout << (colorsEnabled() ? Color::DIM : "");
        cout << "  \u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500"
                "\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\u2500\n";
        cout << (colorsEnabled() ? Color::RESET : "");

        string input = readInput("  Type  \"agree\"  to accept and continue.\n"
                                  "  Type  \"exit\"   to quit.\n"
                                  "  \u2192  ");

        if (input.empty() && cin.eof()) {
            cout << "\n";
            return false;
        }

        input = normalize(input);

        if (input == "agree") {
            cfg.set<int>("disclaimer_accepted", 1);
            cfg.save();
            UI::clearScreen();
            return true;
        }

        if (isQuit(input)) { handleQuit(); return false; }
        if (isBack(input)) { cout << "\n"; return false; }
        if (isMenu(input)) { cout << "\n"; return false; }

        // any other input — redisplay without error message
    }
}