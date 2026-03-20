/*
 *  tguide — UI_settings.cpp
 *  written by voidoxin
 *
 *  Settings screen — runtime configuration editor.
 *  Future implementation will display current config values loaded from
 *  ConfigManager and allow the user to edit: theme, default platform,
 *  auto-update toggle, and database/config path overrides.
 *  Connects upward to svc_settings → L0-core ConfigManager once
 *  L2-services is implemented.
 */

#include "../includes/UI_settings.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include <iostream>

using namespace std;

void UISettings::show() {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("settings");
    UI::printDivider();

    cout << "\n"
         << Color::DIM << "  settings" << Color::RESET
         << Color::CYAN << " — " << Color::RESET
         << Color::BOLD << "coming soon" << Color::RESET
         << "\n\n"
         << Color::DIM
         << "  configure theme, platform defaults, and update behavior.\n"
         << "  this screen is not yet implemented.\n"
         << Color::RESET
         << "\n";

    UI::printDivider();
    cout << "\n";

    pause();
}