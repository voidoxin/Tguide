/*
 *  tguide — UI_savedScripts.cpp
 *  written by voidoxin
 *
 *  Saved Scripts screen — personal script library.
 *  Future implementation will list all scripts generated or imported
 *  by the user, support view / edit / delete / export-to-file,
 *  and allow labeling by target type or tool category.
 *  Connects upward to svc_savedScripts once L2-services is implemented.
 */

#include "../includes/UI_savedScripts.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include <iostream>

using namespace std;

void UISavedScripts::show() {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("saved scripts");
    UI::printDivider();

    cout << "\n"
         << Color::DIM << "  saved scripts" << Color::RESET
         << Color::CYAN << " — " << Color::RESET
         << Color::BOLD << "coming soon" << Color::RESET
         << "\n\n"
         << Color::DIM
         << "  view, edit, and export your saved scripts.\n"
         << "  this screen is not yet implemented.\n"
         << Color::RESET
         << "\n";

    UI::printDivider();
    cout << "\n";

    pause();
}