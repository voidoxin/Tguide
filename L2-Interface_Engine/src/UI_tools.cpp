/*
 *  tguide — UI_tools.cpp
 *  written by voidoxin
 *
 *  Tools screen — browse the security tools reference.
 *  Future implementation will display tool list with search,
 *  per-tool flag breakdown, usage templates, and platform notes.
 *  Connects upward to svc_tools once L2-services is implemented.
 */

#include "../includes/UI_tools.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include <iostream>

using namespace std;

void UITools::show() {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("tools");
    UI::printDivider();

    cout << "\n"
         << Color::DIM << "  tools reference" << Color::RESET
         << Color::CYAN << " — " << Color::RESET
         << Color::BOLD << "coming soon" << Color::RESET
         << "\n\n"
         << Color::DIM
         << "  browse security tools, flags, and usage templates.\n"
         << "  this screen is not yet implemented.\n"
         << Color::RESET
         << "\n";

    UI::printDivider();
    cout << "\n";

    pause();
}