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

    // Language
    std::string getLanguage(const ConfigManager& cfg);           // returns raw value like "en"
    bool setLanguage(ConfigManager& cfg, const std::string& lang);

    // DB Update Behavior
    std::string getDbUpdateBehavior(const ConfigManager& cfg);   // returns raw value like "ask_me"
    bool setDbUpdateBehavior(ConfigManager& cfg, const std::string& behavior);

    // Extension Priority
    std::string getExtensionPriority(const ConfigManager& cfg);  // returns raw value like "color"
    bool setExtensionPriority(ConfigManager& cfg, const std::string& priority);

    // Display label helpers
    std::string dbBehaviorToLabel(const std::string& value);     // "ask_me" → "Ask Me"
    std::string extPriorityToLabel(const std::string& value);    // "color" → "Color + Label"
    std::string langToLabel(const std::string& value);           // "en" → "English (en)"

}