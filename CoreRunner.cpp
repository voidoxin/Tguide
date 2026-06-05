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

    // ── register error callbacks before any L0 calls ─────────────────────
    // L0-core (DatabaseManager, UserDataManager, ConfigManager, PathResolver)
    // uses g_errorHandler to report errors. Must be set up first so that no
    // error is silently swallowed due to a null std::function member.
    g_errorHandler = { UI_fatal, UI_errors, UI_attention };

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

    // ── verify database accessibility at boot ──────────────────────────────
    // A single ToolD construction validates the database path + triggers
    // resolveDatabase(). The old 5-instance pattern was redundant — service
    // layer creates its own ToolD/ModuD/etc when needed.
    ToolD _bootCheck(PathResolver::dbFile().string());
    (void)_bootCheck;

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