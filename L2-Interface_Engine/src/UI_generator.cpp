/*
 *  tguide — UI_generator.cpp
 *  written by voidoxin
 *
 *  Script Generator screen — interactive command builder.
 *  Future implementation will walk the user through selecting a tool,
 *  filling in flag values, choosing a target, and producing a
 *  ready-to-run shell command or multi-step script.
 *  Connects upward to svc_generator once L2-services is implemented.
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
         << Color::DIM << "  script generator" << Color::RESET
         << Color::CYAN << " — " << Color::RESET
         << Color::BOLD << "coming soon" << Color::RESET
         << "\n\n"
         << Color::DIM
         << "  build commands interactively from tool templates.\n"
         << "  this screen is not yet implemented.\n"
         << Color::RESET
         << "\n";

    UI::printDivider();
    cout << "\n";

    waitForEnter();
}