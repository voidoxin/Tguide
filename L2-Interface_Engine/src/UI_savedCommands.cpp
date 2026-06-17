/*
 *  tguide — UI_savedCommands.cpp
 *  written by voidoxin
 *
 *  Saved Commands screen — personal command library.
 *  Allows users to browse, view, add, edit, delete, and preview
 *  their saved commands.  Connects to svc_savedCommands for all
 *  data access and persistence.
 */

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

#include "../includes/svc_generator.h"       // sanitizeInput
#include "../includes/svc_savedCommands.h"
#include "../includes/svc_strings.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include "../includes/UI_paginator.h"
#include "../includes/UI_savedCommands.h"
#include "../includes/UI_utils.h"

using namespace std;

// ── forward declarations ────────────────────────────────────────────────────
static void showCommandList();
static void showCommandDetail(const SvcDTO::SavedCommandDTO& cmd);
static void editCommandNote(const SvcDTO::SavedCommandDTO& cmd);
static bool deleteCommand(const SvcDTO::SavedCommandDTO& cmd);
static void addNewCommand();

// ==================== HELPERS ====================

// Return a short human-readable label for a saved command (used in breadcrumbs)
static string commandDisplayLabel(const SvcDTO::SavedCommandDTO& cmd,
                                   size_t maxLen = 40) {
    if (!cmd.note.empty())
        return cmd.note;
    string truncated = cmd.command.substr(0, maxLen);
    if (cmd.command.size() > maxLen)
        truncated += "\u2026";
    return truncated;
}

// Draw a boxed preview around the given command string.
// Pattern from UI_tools.cpp — showTemplateFill()
static void drawPreviewBox(const string& command) {
    string label = "  $ " + command;
    size_t inner = label.size() + 2;    // 2 trailing spaces

    string hline;
    hline.reserve(inner * 3);
    for (size_t i = 0; i < inner; ++i)
        hline += "\u2500";

    cout << "  \u250c" << hline        << "\u2510\n"
         << "  \u2502 " << label << "  " << "\u2502\n"
         << "  \u2514" << hline        << "\u2518\n\n";
}

// ==================== COMMAND LIST ====================

static void showCommandList() {
    while (true) {
        // Re-fetch every time we enter or return from a detail view
        vector<SvcDTO::SavedCommandDTO> commands =
            SvcSavedCommands::getAllCommands();

        if (commands.empty()) {
            UI::clearScreen();
            UI::printBanner();
            UI::printBreadcrumb("saved commands \u203a list");
            UI::printDivider();
            cout << "\n  " << Strings::get(StringID::SAVED_COMMANDS_EMPTY)
                 << "\n\n";
            UI::printDivider();
            cout << "\n";
            waitForEnter();
            return;
        }

        // ── build display lines ───────────────────────────────────────
        vector<string> lines;
        lines.reserve(commands.size());
        for (const SvcDTO::SavedCommandDTO& cmd : commands) {
            string display = cmd.note.empty()
                ? cmd.command.substr(0, 60)
                    + (cmd.command.size() > 60 ? "\u2026" : "")
                : cmd.note;

            string line;
            if (colorsEnabled()) {
                line += string(Color::RESET) + Color::BOLD + Color::CYAN;
                line += display;
                line += string(Color::RESET) + "  " + Color::DIM;
                line += "\u2014 " + cmd.tool_name;
                line += Color::RESET;
            } else {
                line = display + "  \u2014 " + cmd.tool_name;
            }
            lines.push_back(line);
        }

        string    crumb = "saved commands \u203a list";
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

            showCommandDetail(commands[static_cast<size_t>(idx)]);
            // After returning from detail, refresh the list from storage
            refreshed = true;
        }
    }
}

// ==================== COMMAND DETAIL ====================

static void showCommandDetail(const SvcDTO::SavedCommandDTO& cmd) {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("saved commands \u203a list \u203a "
                            + commandDisplayLabel(cmd));
        UI::printDivider();

        // ── command info ──
        cout << "\n  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << "command:"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n"
             << "  " << cmd.command << "\n\n";

        cout << "  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << "tool:"
             << (colorsEnabled() ? Color::RESET : "")
             << "  " << cmd.tool_name << "\n\n";

        cout << "  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << "note:"
             << (colorsEnabled() ? Color::RESET : "")
             << "  " << (cmd.note.empty() ? "(none)" : cmd.note) << "\n\n";

        // ── preview box ──
        cout << "  "
             << (colorsEnabled() ? string(Color::BOLD) : "")
             << Strings::get(StringID::SAVED_PREVIEW) << ":"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";
        drawPreviewBox(cmd.command);

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
            editCommandNote(cmd);
            // Return so the list refreshes
            return;
        }

        if (input == "d" || input == "D") {
            if (deleteCommand(cmd))
                return;     // deleted — go back to refreshed list
            // deletion cancelled or failed — stay on detail
            continue;
        }

        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
    }
}

// ==================== EDIT NOTE ====================

static void editCommandNote(const SvcDTO::SavedCommandDTO& cmd) {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("saved commands \u203a list \u203a "
                        + commandDisplayLabel(cmd) + " \u203a edit note");
    UI::printDivider();

    cout << "\n  current note: "
         << (colorsEnabled() ? Color::CYAN : "")
         << (cmd.note.empty() ? "(none)" : cmd.note)
         << (colorsEnabled() ? Color::RESET : "")
         << "\n\n";

    cout << "  " << Strings::get(StringID::SAVED_NOTE_PROMPT)
         << " (enter to keep current):\n";
    string newNote = readInput("  \u2192 ");
    if (isQuit(newNote)) { handleQuit(); return; }
    if (isBack(newNote)) return;

    // If empty, keep current note
    if (newNote.empty())
        newNote = cmd.note;

    bool ok = SvcSavedCommands::updateCommand(
        cmd.id, cmd.tool_id, cmd.command, newNote);

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

// ==================== DELETE COMMAND ====================

static bool deleteCommand(const SvcDTO::SavedCommandDTO& cmd) {
    cout << "\n  " << Strings::get(StringID::SAVED_CONFIRM_DELETE) << " ";
    string confirm = readInput("");
    if (confirm.empty()) return false;
    if (isQuit(confirm)) { handleQuit(); return false; }
    if (isBack(confirm)) return false;

    if (confirm == "y" || confirm == "Y") {
        bool ok = SvcSavedCommands::deleteCommand(cmd.id);
        if (ok) {
            cout << "\n  " << (colorsEnabled() ? Color::GREEN : "")
                 << Strings::get(StringID::SAVED_COMMANDS_DELETED)
                 << (colorsEnabled() ? Color::RESET : "") << "\n\n";
            waitForEnter();
            return true;
        } else {
            cout << "\n  " << (colorsEnabled() ? Color::YELLOW : "")
                 << "! failed to delete command."
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

// ==================== ADD NEW COMMAND ====================

static void addNewCommand() {
    string note, command;

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("saved commands \u203a add");
        UI::printDivider();

        // ── form ──
        cout << "\n  "
             << (colorsEnabled() ? Color::DIM : "")
             << "enter the command details below."
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        // Note (optional)
        cout << "  " << Strings::get(StringID::SAVED_NOTE_PROMPT)
             << " (optional)";
        if (!note.empty())
            cout << " [" << note << "]";
        cout << ":\n";
        {
            string inp = readInput("  \u2192 ");
            if (!inp.empty()) {
                if (isQuit(inp)) { handleQuit(); return; }
                if (isBack(inp)) return;
                note = sanitizeInput(inp);
            }
        }

        // Command (required)
        cout << "\n  " << Strings::get(StringID::SAVED_COMMAND_PROMPT)
             << " (required):\n";
        {
            string inp = readInput("  \u2192 ");
            if (inp.empty()) {
                cout << "\n  "
                     << (colorsEnabled() ? Color::YELLOW : "")
                     << "command cannot be empty."
                     << (colorsEnabled() ? Color::RESET : "") << "\n\n";
                waitForEnter();
                continue;
            }
            if (isQuit(inp)) { handleQuit(); return; }
            if (isBack(inp)) return;
            command = sanitizeInput(inp);
        }

        // ── confirmation / preview ─────────────────────────────────────
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("saved commands \u203a add \u203a preview");
        UI::printDivider();

        cout << "\n  "
             << (colorsEnabled() ? string(Color::BOLD) : "")
             << Strings::get(StringID::SAVED_PREVIEW) << ":"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";
        drawPreviewBox(command);

        // Show entered metadata
        cout << "  "
             << (colorsEnabled() ? string(Color::BOLD) + Color::CYAN : "")
             << Strings::get(StringID::SAVED_NOTE_PROMPT) << ":"
             << (colorsEnabled() ? Color::RESET : "")
             << " " << (note.empty() ? "(none)" : note) << "\n\n";

        // ── save / cancel ──
        cout << (colorsEnabled() ? Color::DIM : "")
             << "  [s] " << Strings::get(StringID::SAVED_COMMANDS_ADD)
             << "   [0] " << Strings::get(StringID::TOOLS_BACK)
             << (colorsEnabled() ? Color::RESET : "")
             << "\n\n";

        string inp = readInput("  \u2192 ");
        if (inp.empty()) continue;
        if (isQuit(inp)) { handleQuit(); return; }

        if (inp == "s" || inp == "S") {
            int id = SvcSavedCommands::saveCommand(0, command, note);
            if (id != -1) {
                cout << "\n  " << (colorsEnabled() ? Color::GREEN : "")
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

        if (isBack(inp)) return;

        cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE) << "\n";
        waitForEnter();
    }
}

// ==================== ENTRY POINT ====================

void UISavedCommands::show() {
    const vector<string> opts = {
        Strings::get(StringID::SAVED_COMMANDS_LIST),
        Strings::get(StringID::SAVED_COMMANDS_ADD)
    };

    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("saved commands");
        UI::printDivider();

        cout << "\n"
             << "  \u251C\u2500 \u25C9  "
             << Strings::get(StringID::SAVED_COMMANDS_LIST)
             << "   [1]\n"
             << "  \u251C\u2500 \u002b  "
             << Strings::get(StringID::SAVED_COMMANDS_ADD)
             << "        [2]\n"
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
            showCommandList();
        } else if (idx == 1) {
            addNewCommand();
        } else {
            cout << "  " << Strings::get(StringID::TOOLS_INVALID_CHOICE)
                 << "\n";
            waitForEnter();
        }
    }
}
