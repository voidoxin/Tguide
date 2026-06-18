/*
 *  tguide — UI_settings.h
 *  written by voidoxin
 *
 *  Settings screen — runtime configuration editor.
 *  Displays and modifies config values: theme, default platform,
 *  auto-update preference, and database path overrides.
 *  Connects to svc_settings → L0-core ConfigManager.
 */

#pragma once

class ConfigManager;

namespace UISettings {

    // render the Settings screen — blocks until user navigates back
    void show(ConfigManager& cfg);

    // render the Database Management sub-screen with rollback options
    void showDatabaseMenu();

}