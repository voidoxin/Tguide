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
#include <cstdint>
#include <string>

class ConfigManager;

namespace SvcSettings {

    // Color (existing)
    bool getColorEnabled(const ConfigManager& cfg);
    bool setColorEnabled(ConfigManager& cfg, bool enabled);

    // Database info struct
    struct DbInfo {
        std::string version;      // from DBCacheManager
        uintmax_t   fileSize;     // in bytes
        int         tableCount;
        int         rowCount;
        bool        hasBackup;
    };

    // Database management
    DbInfo  getDbInfo();
    bool    triggerDbUpdate();

}