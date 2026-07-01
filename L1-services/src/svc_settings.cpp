/*
 *  tguide — svc_settings.cpp
 *  written by voidoxin
 *
 *  Service layer for the Settings screen.
 *  Connects UI_settings → L0-core ConfigManager.
 */

#include "../includes/svc_settings.h"
#include "../../L0-core/include/config_manager.h"
#include "../../L0-core/include/DBResolver.h"
#include "../../L0-core/include/db_cache_manager.h"
#include "../../L0-core/include/path_resolver.h"

namespace SvcSettings {

// ── Color (existing) ──────────────────────────────────
bool getColorEnabled(const ConfigManager& cfg) {
    return cfg.get<int>("colors", 1) != 0;
}

bool setColorEnabled(ConfigManager& cfg, bool enabled) {
    cfg.set<int>("colors", enabled ? 1 : 0);
    return cfg.save();
}

// ── Database info ──────────────────────────────────────
DbInfo getDbInfo() {
    DbInfo info;
    std::string dbPath = PathResolver::dbFile().string();

    info.version   = DBCacheManager::instance().getLastSeenVersion();
    info.hasBackup = DBCacheManager::instance().hasBackup();

    auto dbinfo = DBResolver::instance().getDatabaseInfo(dbPath);
    info.fileSize   = dbinfo.fileSize;
    info.tableCount = dbinfo.tableCount;
    info.rowCount   = dbinfo.rowCount;

    return info;
}

// ── Manual DB update ──────────────────────────────────
bool triggerDbUpdate() {
    std::string dbPath = PathResolver::dbFile().string();
    return DBResolver::instance().manualUpdate(dbPath);
}

// ── Language ────────────────────────────────────────
std::string getLanguage(const ConfigManager& cfg) {
    return cfg.get<std::string>("lang", "en");
}

bool setLanguage(ConfigManager& cfg, const std::string& lang) {
    cfg.set<std::string>("lang", lang);
    return cfg.save();
}

std::string langToLabel(const std::string& value) {
    if (value == "en") return "English (en)";
    return value; // fallback
}

// ── DB Update Behavior ──────────────────────────────
std::string getDbUpdateBehavior(const ConfigManager& cfg) {
    return cfg.get<std::string>("db_update_behavior", "ask_me");
}

bool setDbUpdateBehavior(ConfigManager& cfg, const std::string& behavior) {
    cfg.set<std::string>("db_update_behavior", behavior);
    return cfg.save();
}

std::string dbBehaviorToLabel(const std::string& value) {
    if (value == "never")  return "Never";
    if (value == "ask_me") return "Ask Me";
    if (value == "auto")   return "Auto";
    return value;
}

// ── Extension Priority ──────────────────────────────
std::string getExtensionPriority(const ConfigManager& cfg) {
    return cfg.get<std::string>("extension_priority", "color");
}

bool setExtensionPriority(ConfigManager& cfg, const std::string& priority) {
    cfg.set<std::string>("extension_priority", priority);
    return cfg.save();
}

std::string extPriorityToLabel(const std::string& value) {
    if (value == "db_only")  return "Database Data Only";
    if (value == "color")    return "Color + Label";
    if (value == "ext_only") return "Extension Data Only";
    return value;
}

} // namespace SvcSettings