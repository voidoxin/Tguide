/*
 *  tguide — UI_utils.cpp
 *  written by voidoxin
 */

#include <iostream>

#include "../includes/UI_colors.h"
#include "../includes/UI_utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

using namespace std;

void UI::clearScreen() noexcept {
#ifdef _WIN32
    system("cls");
#else
    // ANSI escape — no system() call needed
    cout << "\033[2J\033[H";
#endif
}

void UI::printBanner() {
    if (colorsEnabled()) cout << Color::CYAN << Color::BOLD;
    cout << R"(
  ████████╗ ██████╗ ██╗   ██╗██╗██████╗ ███████╗
     ██╔══╝██╔════╝ ██║   ██║██║██╔══██╗██╔════╝
     ██║   ██║  ███╗██║   ██║██║██║  ██║█████╗
     ██║   ██║   ██║██║   ██║██║██║  ██║██╔══╝
     ██║   ╚██████╔╝╚██████╔╝██║██████╔╝███████╗
     ╚═╝    ╚═════╝  ╚═════╝ ╚═╝╚═════╝ ╚══════╝
)" << (colorsEnabled() ? Color::RESET : "");

    if (colorsEnabled()) cout << Color::DIM;
    cout << "          security tools reference — by voidoxin\n"
         << (colorsEnabled() ? Color::RESET : "") << "\n";
}

void UI::printDivider() {
    if (colorsEnabled()) cout << Color::DIM;
    cout << "  ──────────────────────────────────────────────\n"
         << (colorsEnabled() ? Color::RESET : "");
}

void UI::printBreadcrumb(const std::string& section) {
    if (colorsEnabled()) cout << Color::DIM;
    cout << "  tguide"
         << (colorsEnabled() ? Color::RESET : "")
         << (colorsEnabled() ? Color::CYAN : "") << " › "
         << (colorsEnabled() ? Color::RESET : "")
         << (colorsEnabled() ? Color::BOLD : "") << section
         << (colorsEnabled() ? Color::RESET : "")
         << "\n\n";
}
