/*
 *  tguide — UI_errorHandling.cpp
 *  written by voidoxin
 */

#include "../includes/UI_errorHandling.h"
#include "../includes/UI_colors.h"
#include <iostream>

using namespace std;

void pause() {
#ifdef _WIN32
    system("pause");
#else
    cout << Color::DIM << "  press enter to continue..." << Color::RESET;
    cin.ignore();
    cin.get();
#endif
}

void UI_fatal(const std::string& msg) {
    cerr << "\n"
         << Color::RED << Color::BOLD << "  [FATAL] " << Color::RESET
         << Color::RED << msg << Color::RESET
         << "\n\n";
    pause();
    // caller handles exit
}

void UI_errors(const std::string& msg) {
    cerr << Color::YELLOW << "  [error] " << Color::RESET
         << msg << "\n";
}

char UI_attention(const std::string& msg) {
    cout << "\n"
         << Color::YELLOW << Color::BOLD << "  [!] " << Color::RESET
         << msg << "\n"
         << Color::DIM   << "  → " << Color::RESET;
    char c;
    cin >> c;
    cin.ignore();
    return c;
}