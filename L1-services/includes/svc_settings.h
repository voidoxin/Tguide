/*
 *  tguide — svc_settings.h
 *  written by voidoxin
 *
 *  Service layer for the Settings screen.
 *  Handles all business logic between UI_settings and L0-core:
 *    - read current config values from ConfigManager
 *    - validate and write updated config values
 *    - handle config file reload after changes
 */

#pragma once

class ConfigManager;

namespace SvcSettings {

    bool getColorEnabled(const ConfigManager& cfg);
    bool setColorEnabled(ConfigManager& cfg, bool enabled);

}