/*
 *  tguide — UI_settings.h
 *  written by voidoxin
 *
 *  Settings screen — runtime configuration editor.
 *  Displays and modifies config values: theme, default platform,
 *  auto-update preference, and database path overrides.
 *  Will connect to svc_settings → L0-core ConfigManager.
 */

#pragma once

namespace UISettings {

    // render the Settings screen — blocks until user navigates back
    void show();

    // render the Database Management sub-screen with rollback options
    void showDatabaseMenu();

}