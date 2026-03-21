/*
 *  tguide — UI_errorHandling.cpp
 *  fatal / recoverable error display and acknowledgement primitives
 *
 *  written by voidoxin
 */

#include "../includes/UI_errorHandling.h"
#include "../includes/UI_colors.h"
#include <iostream>
#include <limits>

using namespace std;

// ==================== PAUSE ====================

// standalone wait — used when no message is needed but screen must not advance
void pause() {
    if (cin.eof()) return;
    cout << (colorsEnabled() ? Color::DIM : "")
         << "  press enter to continue..."
         << (colorsEnabled() ? Color::RESET : "")
         << flush;
    cin.clear();
    cin.get();
}

// ==================== FATAL ====================

// prints [FATAL] and waits with "press enter to exit..." — does NOT call exit()
// clearScreen() must NEVER be called by any caller after this returns
void UI_fatal(const std::string& msg) {
    cerr << "\n"
         << (colorsEnabled() ? Color::RED  : "")
         << (colorsEnabled() ? Color::BOLD : "")
         << "  [FATAL] "
         << (colorsEnabled() ? Color::RESET : "")
         << (colorsEnabled() ? Color::RED  : "")
         << msg
         << (colorsEnabled() ? Color::RESET : "")
         << "\n\n";

    if (!cin.eof()) {
        cerr << (colorsEnabled() ? Color::DIM : "")
             << "  press enter to exit..."
             << (colorsEnabled() ? Color::RESET : "")
             << flush;
        cin.clear();
        cin.get();
    }

    cerr << "\n";
    // caller returns 1 from main() — do not call exit() here
}

// ==================== RECOVERABLE ====================

// prints [error] and waits for acknowledgement — pause is built in
// callers do NOT need a separate pause() call after this
void UI_errors(const std::string& msg) {
    cerr << (colorsEnabled() ? Color::YELLOW : "")
         << "  [error] "
         << (colorsEnabled() ? Color::RESET : "")
         << msg << "\n";
    pause();
}

// ==================== ATTENTION ====================

// prints [!] prompt and reads one char response — returns 0 on EOF or failure
char UI_attention(const std::string& msg) {
    cout << "\n"
         << (colorsEnabled() ? Color::YELLOW : "")
         << (colorsEnabled() ? Color::BOLD   : "")
         << "  [!] "
         << (colorsEnabled() ? Color::RESET  : "")
         << msg << "\n"
         << (colorsEnabled() ? Color::DIM    : "")
         << "  → "
         << (colorsEnabled() ? Color::RESET  : "");

    // initialize to 0 — caller must treat 0 as "no input / invalid"
    char c = 0;
    if (!(cin >> c)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        return 0;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return c;
}