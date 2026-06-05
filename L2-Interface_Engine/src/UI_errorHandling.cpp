/*
 *  tguide — UI_errorHandling.cpp
 *  fatal / recoverable error display and acknowledgement primitives
 *
 *  written by voidoxin
 */

#include "../includes/UI_errorHandling.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_input.h"
#include <iostream>

using namespace std;

// ==================== WAIT ====================

// standalone wait — used when no message is needed but screen must not advance
void waitForEnter() {
    if (cin.eof()) return;
    cout << (colorsEnabled() ? Color::DIM : "")
         << "  press enter to continue..."
         << (colorsEnabled() ? Color::RESET : "")
         << flush;
    readInput("");
}

// ==================== FATAL ====================

// prints [FATAL] and waits with "press enter to exit..." — does NOT call exit()
// clearScreen() must NEVER be called by any caller after this returns
void UI_fatal(const std::string& msg) {
    cout << "\n"
         << (colorsEnabled() ? Color::RED  : "")
         << (colorsEnabled() ? Color::BOLD : "")
         << "  [FATAL] "
         << (colorsEnabled() ? Color::RESET : "")
         << (colorsEnabled() ? Color::RED  : "")
         << msg
         << (colorsEnabled() ? Color::RESET : "")
         << "\n\n";

    if (!cin.eof()) {
        cout << (colorsEnabled() ? Color::DIM : "")
             << "  press enter to exit..."
             << (colorsEnabled() ? Color::RESET : "")
             << flush;
        readInput("");
    }

    cout << "\n";
    // caller returns 1 from main() — do not call exit() here
}

// ==================== RECOVERABLE ====================

// prints [error] and waits for acknowledgement — pause is built in
// callers do NOT need a separate waitForEnter() call after this
void UI_errors(const std::string& msg) {
    cout << (colorsEnabled() ? Color::YELLOW : "")
         << "  [error] "
         << (colorsEnabled() ? Color::RESET : "")
         << msg << "\n";
    waitForEnter();
}

// ==================== ATTENTION ====================

// prints [!] prompt and reads one char response — returns 0 on EOF or failure
char UI_attention(const std::string& msg) {
    cout << "\n"
         << (colorsEnabled() ? Color::YELLOW : "")
         << (colorsEnabled() ? Color::BOLD   : "")
         << "  [!] "
         << (colorsEnabled() ? Color::RESET  : "")
         << msg << "\n";

    string input = readInput("  → ");
    if (input.empty()) return 0;
    return input[0];
}