/*
 *  tguide — CoreRunner.cpp
 *  entry point / bootstrap
 *
 *  written by voidoxin
 */

#include <iostream>
#include <filesystem>
#include "L0-core/include/DatabaseManager.h"
#include "L0-core/include/config_manager.h"
#include "L0-core/include/path_resolver.h"
#include "L0-core/include/db_cache_manager.h"
#include "L2-Interface_Engine/includes/UI_errorHandling.h"
#include "L2-Interface_Engine/includes/UI_Engine.h"
                                                  using namespace std;
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    // ── check write access before touching anything ────────────────────────
    // on Linux, /etc/ and /usr/share/ require root
    // on Termux and Windows this always passes
    if (!PathResolver::hasWriteAccess()) {
        UI_fatal("tguide requires root privileges on Linux.\n"
                 "Please run with: sudo tguide");
        return 1;
    }

    // ── create required directories ────────────────────────────────────────
    if (!PathResolver::createDirs()) {
        UI_fatal("Failed to create required directories.\n"
                 "Check permissions or run with sudo.");
        return 1;
    }

    // ── load config ────────────────────────────────────────────────────────
    ConfigManager cfg(PathResolver::configFile().string());

    // ── init cache before any DB class is constructed ──────────────────────
    // resolveDatabase() calls DBCache internally — must be ready first
    DBCache::init(PathResolver::cacheFile().string());

    // ── init backup manager (dev only — removed before release) ───────────
    BackupManager::init(PathResolver::backupFile().string());

    // ── bring up all db classes ────────────────────────────────────────────
    // resolveDatabase() is idempotent — all classes share the same resolved path
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