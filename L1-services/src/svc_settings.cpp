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

} // namespace SvcSettings