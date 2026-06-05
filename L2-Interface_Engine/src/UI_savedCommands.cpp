/*
 *  tguide — UI_savedCommands.cpp
 *  written by voidoxin
 *
 *  Saved Commands screen — personal command library.
 *  Future implementation will list all user-saved commands with
 *  labels and tags, support add / edit / delete / copy-to-clipboard,
 *  and allow quick search by keyword or tool name.
 *  Connects upward to svc_savedCommands once L2-services is implemented.
 */

#include "../includes/UI_savedCommands.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include <iostream>

using namespace std;

void UISavedCommands::show() {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("saved commands");
    UI::printDivider();

    cout << "\n"
         << Color::DIM << "  saved commands" << Color::RESET
         << Color::CYAN << " — " << Color::RESET
         << Color::BOLD << "coming soon" << Color::RESET
         << "\n\n"
         << Color::DIM
         << "  your personal command library — save, label, and reuse commands.\n"
         << "  this screen is not yet implemented.\n"
         << Color::RESET
         << "\n";

    UI::printDivider();
    cout << "\n";

    waitForEnter();
}