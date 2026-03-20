/*
 *  tguide — UI_errorHandling.cpp
 *  written by voidoxin
 */

#include "../includes/UI_errorHandling.h"
#include "../includes/UI_colors.h"
#include <iostream>
#include <limits>

using namespace std;

void pause() {
#ifdef _WIN32
    system("pause");                              #else
    if (!cin.eof()) {                                     cout << Color::DIM << "  press enter to continue..." << Color::RESET;
        cin.get();
    }
#endif
}

void UI_fatal(const std::string& msg) {
    cerr << "\n"
         << Color::RED << Color::BOLD << "  [FATAL] " << Color::RESET
         << Color::RED << msg << Color::RESET
         << "\n\n";
    pause();
    // caller handles exit — don't call exit() here
}

void UI_errors(const std::string& msg) {
    cerr << Color::YELLOW << "  [error] " << Color::RESET
         << msg << "\n";
}

char UI_attention(const std::string& msg) {
    cout << "\n"
         << Color::YELLOW << Color::BOLD << "  [!] " << Color::RESET
         << msg << "\n"
         << Color::DIM << "  → " << Color::RESET;

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