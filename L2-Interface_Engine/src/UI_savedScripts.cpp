/*
 *  tguide — UI_savedScripts.cpp
 *  written by voidoxin
 *
 *  Saved Scripts screen — personal script library.
 *  Allows users to browse, view, edit notes, and delete
 *  their saved scripts.  Connects to svc_savedScripts for all
 *  data access and persistence.
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "../includes/svc_savedScripts.h"
#include "../includes/svc_strings.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include "../includes/UI_paginator.h"
#include "../includes/UI_savedScripts.h"
#include "../includes/UI_utils.h"

using namespace std;

// ── forward declarations ────────────────────────────────────────────────────
static void showScriptList();
static void showScriptDetail(const SvcDTO::SavedScriptDTO& script);
static void editScriptNote(const SvcDTO::SavedScriptDTO& script);
static bool deleteScript(const SvcDTO::SavedScriptDTO& script);

// ==================== HELPERS ====================

// Return a short human-readable label for a saved script (used in breadcrumbs)
static string scriptDisplayLabel(const SvcDTO::SavedScriptDTO& script,
                                  size_t maxLen = 40) {
    if (!script.note.empty())
        return script.note;
    string truncated = script.name.substr(0, maxLen);
    if (script.name.size() > maxLen)
        truncated += "\u2026";
    return truncated;
}

// ==================== SCRIPT LIST ====================

static void showScriptList() {
    while (true) {
        // Re-fetch every time we enter or return from a detail view
        vector<SvcDTO::SavedScriptDTO> scripts =
            SvcSavedScripts::getAllScripts();

        if (scripts.empty()) {
            UI::clearScreen();
            UI::printBanner();
            UI::printBreadcrumb("saved scripts \u203a list");
            UI::printDivider();
            cout << "\n  " << Strings::get(StringID::SAVED_SCRIPTS_EMPTY)
                 << "\n\n";
            UI::printDivider();
            cout << "\n";
            waitForEnter();
            return;
        }

        // ── build display lines ───────────────────────────────────────
        vector<string> lines;
        lines.reserve(scripts.size());
        for (const SvcDTO::SavedScriptDTO& script : scripts) {
            string display = script.note.empty()
                ? script.name.substr(0, 60)
                    + (script.name.size() > 60 ? "\u2026" : "")
                : script.note;

            string line;
            if (colorsEnabled()) {
                line += string(Color::RESET) + Color::BOLD + Color::CYAN;
                line += display;
                line += string(Color::RESET) + "  " + Color::DIM;
                line += "\u2014 " + script.name;
                line += Color::RESET;
            } else {
                line = display + "  \u2014 " + script.name;
            }
            lines.push_back(line);
        }

        string    crumb = "saved scripts \u203a list";
        Paginator pager(lines, true);

        bool refreshed = false;
        while (!refreshed) {
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
                cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE)
                     << "\n";
                waitForEnter();
                continue;
            }

            showScriptDetail(scripts[static_cast<size_t>(idx)]);
            // After returning from detail, refresh the list from storage
            refreshed = true;
        }
    }
}

// ==================== SCRIPT DETAIL ====================

static void showScriptDetail(const SvcDTO::SavedScriptDTO& script) {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("saved scripts \u203a list \u203a "
                            + scriptDisplayLabel(script));
        UI::printDivider();

        // ── script info ──
        cout << "\n  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << Strings::get(StringID::SAVED_SCRIPT_NAME_PROMPT) << ":"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n"
             << "  " << script.name << "\n\n";

        cout << "  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << Strings::get(StringID::SAVED_SCRIPT_PATH_PROMPT) << ":"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n"
             << "  " << (colorsEnabled() ? Color::DIM : "") << script.path
             << (colorsEnabled() ? Color::RESET : "") << "\n\n";

        cout << "  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << Strings::get(StringID::SAVED_NOTE_PROMPT) << ":"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n"
             << "  " << (script.note.empty() ? "(none)" : script.note)
             << "\n\n";

        // ── options ──
        cout << (colorsEnabled() ? Color::DIM : "")
             << "  [e] " << Strings::get(StringID::SAVED_EDIT_NOTE) << "\n"
             << "  [d] " << Strings::get(StringID::SAVED_COMMANDS_DELETE)
             << "\n"
             << "  [0] " << Strings::get(StringID::TOOLS_BACK)
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        if (input == "e" || input == "E") {
            editScriptNote(script);
            // Return so the list refreshes
            return;
        }

        if (input == "d" || input == "D") {
            if (deleteScript(script))
                return;     // deleted — go back to refreshed list
            // deletion cancelled or failed — stay on detail
            continue;
        }

        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
    }
}

// ==================== EDIT NOTE ====================

static void editScriptNote(const SvcDTO::SavedScriptDTO& script) {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("saved scripts \u203a list \u203a "
                        + scriptDisplayLabel(script)
                        + " \u203a edit note");
    UI::printDivider();

    cout << "\n  current note: "
         << (colorsEnabled() ? Color::CYAN : "")
         << (script.note.empty() ? "(none)" : script.note)
         << (colorsEnabled() ? Color::RESET : "")
         << "\n\n";

    cout << "  " << Strings::get(StringID::SAVED_NOTE_PROMPT)
         << " (enter to keep current):\n";
    string newNote = readInput("  \u2192 ");
    if (isQuit(newNote)) { handleQuit(); return; }
    if (isBack(newNote)) return;

    // If empty, keep current note
    if (newNote.empty())
        newNote = script.note;

    bool ok = SvcSavedScripts::updateScript(
        script.id, script.name, script.path, newNote);

    if (ok) {
        cout << "\n  " << (colorsEnabled() ? Color::GREEN : "")
             << "\u2713 note updated."
             << (colorsEnabled() ? Color::RESET : "") << "\n\n";
    } else {
        cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
             << "! failed to update note."
             << (colorsEnabled() ? Color::RESET : "") << "\n\n";
    }
    waitForEnter();
}

// ==================== DELETE SCRIPT ====================

static bool deleteScript(const SvcDTO::SavedScriptDTO& script) {
    cout << "\n  " << Strings::get(StringID::SAVED_CONFIRM_DELETE) << " ";
    string confirm = readInput("");
    if (confirm.empty()) return false;
    if (isQuit(confirm)) { handleQuit(); return false; }
    if (isBack(confirm)) return false;

    if (confirm == "y" || confirm == "Y") {
        bool ok = SvcSavedScripts::deleteScript(script.id);
        if (ok) {
            cout << "\n  " << (colorsEnabled() ? Color::GREEN : "")
                 << Strings::get(StringID::SAVED_COMMANDS_DELETED)
                 << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            waitForEnter();
            return true;
        } else {
            cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
                 << "! failed to delete script."
                 << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            waitForEnter();
            return false;
        }
    }

    // Cancelled
    cout << "\n  "
         << (colorsEnabled() ? Color::DIM : "")
         << "cancelled."
         << (colorsEnabled() ? Color::RESET : "")
         << "\n\n";
    waitForEnter();
    return false;
}

// ==================== ENTRY POINT ====================

void UISavedScripts::show() {
    const vector<string> opts = {
        Strings::get(StringID::SAVED_SCRIPTS_LIST)
    };

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("saved scripts");
        UI::printDivider();

        cout << "\n"
             << "  \u251C\u2500 \u25C9  "
             << Strings::get(StringID::SAVED_SCRIPTS_LIST)
             << "   [1]\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2514\u2500 \u2190  "
             << Strings::get(StringID::TOOLS_BACK)
             << "                 [0]"
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
            showScriptList();
        } else {
            cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE)
                 << "\n";
            waitForEnter();
        }
    }
}
