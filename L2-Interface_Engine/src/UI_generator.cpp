/*
 *  tguide — UI_generator.cpp
 *  written by voidoxin
 *
 *  Script Generator screen — interactive command builder.
 *  Not yet implemented.  Enable with -DTGUIDE_ENABLE_GENERATOR
 *  at compile time to show the menu entry (leads to this stub).
 *  Connects upward to svc_generator (L1-services).
 */

#include "../includes/UI_generator.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include <iostream>

// SECURITY: all placeholder inputs MUST be wrapped with
// sanitizeInput() from svc_generator.h before being
// passed to any command-building function.
// See: .ai/security.md — sanitizeInput rule

using namespace std;

void UIGenerator::show() {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("script generator");
    UI::printDivider();

    cout << "\n"
         << Color::DIM
         << "  the script generator is not available in this build.\n"
         << "  enable it with -DTGUIDE_ENABLE_GENERATOR at compile time.\n"
         << Color::RESET
         << "\n";

    UI::printDivider();
    cout << "\n";

    waitForEnter();
}