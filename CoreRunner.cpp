/*
 *  tguide — CoreRunner.cpp
 *  entry point / bootstrap
 *
 *  written by voidoxin
 */

#include <iostream>
#include <filesystem>
#include "L0-core/include/DatabaseManager.h"
#include "L0-core/include/ErrorHandler.h"
#include "L0-core/include/config_manager.h"
#include "L0-core/include/path_resolver.h"
#include "L0-core/include/db_cache_manager.h"
#include "L0-core/include/UserDataManager.h"
#include "L2-Interface_Engine/includes/UI_errorHandling.h"
#include "L2-Interface_Engine/includes/UI_colors.h"
#include "L2-Interface_Engine/includes/UI_disclaimer.h"
#include "L2-Interface_Engine/includes/UI_Engine.h"
using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    // ── create required directories ────────────────────────────────────────
    // fatal — /etc/tguide and /usr/share/tguide require root on Linux
    if (!PathResolver::createSystemDirs()) {
        UI_fatal("Failed to create required directories.\n"
                 "Check permissions or run with sudo.");
        return 1;
    }

    // non-fatal — user data dir is always writable, app runs without it
    if (!PathResolver::createUserDirs())
        UI_errors("Failed to create user data directories. "
                  "Saved commands and scripts may be unavailable.");

    // ── load config ────────────────────────────────────────────────────────
    ConfigManager cfg(PathResolver::configFile().string());

    // ── init color toggle from config ──────────────────────────────────────
    // must happen before any UI output so the correct mode is in effect
    bool colors = cfg.get<int>("colors", 1) == 1;
    initColors(colors);

    // ── init user data storage ─────────────────────────────────────────────
    // non-fatal: missing or unreadable files are recreated automatically
    UserDataManager userData(
        PathResolver::savedCommandsFile().string(),
        PathResolver::savedScriptsFile().string()
    );
    if (!userData.load())
        UI_errors("Failed to load user data files. Saved data may be unavailable.");

    // ── legal disclaimer — first run only ──────────────────────────────────
    if (cfg.get<int>("disclaimer_accepted", 0) == 0) {
        if (!UIDisclaimer::show(cfg))
            return 0;
    }

    // ── init cache before any DB class is constructed ──────────────────────
    // resolveDatabase() calls DBCache internally — must be ready first
    DBCache::init(PathResolver::cacheFile().string());

    // ── init backup manager (dev only — removed before release) ───────────
#ifdef TGUIDE_DEV_MODE
    BackupManager::init(PathResolver::backupFile().string());
#endif

    // ── register error callbacks (L0 → L2 bridge) ─────────────────────────
    // L0-core uses these instead of calling UI functions directly,
    // keeping the layer boundary clean.
    g_errorHandler = { UI_fatal, UI_errors, UI_attention };

    // ── bring up all db classes ────────────────────────────────────────────
    // resolveDatabase() is idempotent — s_resolvedCache short-circuits
    // calls 2–5. s_fatal set by the first failure blocks all remaining.
    string db_path = PathResolver::dbFile().string();

    VulnD     vulnDB(db_path);
    ModuD     moduDB(db_path);
    ToolD     toolDB(db_path);
    ToolFlagD flagDB(db_path);
    TemplateD tmplDB(db_path);

    // ── abort on fatal db error ────────────────────────────────────────────
    if (DBFatal()) {
        UI_fatal("Database initialization failed.\n"
                 "Check your internet connection or reinstall tguide.");
        return 1;
    }

    // ── hand off to UI ─────────────────────────────────────────────────────
    UIEngine::start();

    return 0;
}