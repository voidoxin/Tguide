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
    cout << Color::CYAN << Color::BOLD;
    cout << R"(
  ████████╗ ██████╗ ██╗   ██╗██╗██████╗ ███████╗
     ██╔══╝██╔════╝ ██║   ██║██║██╔══██╗██╔════╝
     ██║   ██║  ███╗██║   ██║██║██║  ██║█████╗
     ██║   ██║   ██║██║   ██║██║██║  ██║██╔══╝
     ██║   ╚██████╔╝╚██████╔╝██║██████╔╝███████╗
     ╚═╝    ╚═════╝  ╚═════╝ ╚═╝╚═════╝ ╚══════╝
)" << Color::RESET;

    cout << Color::DIM
         << "          security tools reference — by voidoxin\n"
         << Color::RESET << "\n";
}

void UI::printDivider() {
    cout << Color::DIM
         << "  ──────────────────────────────────────────────\n"
         << Color::RESET;
}

void UI::printBreadcrumb(const std::string& section) {
    cout << Color::DIM << "  tguide"
         << Color::RESET
         << Color::CYAN << " › " << Color::RESET
         << Color::BOLD << section << Color::RESET
         << "\n\n";
}
