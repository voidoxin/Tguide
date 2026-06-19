/*
 *  tguide — CoreRunner.cpp
 *  entry point / bootstrap
 *
 *  written by voidoxin
 */

#include <iostream>
#include <filesystem>
#include <sqlite3.h>
#include "L0-core/include/DatabaseManager.h"
#include "L0-core/include/DBResolver.h"
#include "L0-core/include/ErrorHandler.h"
#include "L0-core/include/config_manager.h"
#include "L0-core/include/path_resolver.h"
#include "L0-core/include/db_cache_manager.h"
#include "L0-core/include/UserDataManager.h"
#include "L0-core/include/cli_parser.h"
#include "L0-core/include/session_flags.h"
#include "L2-Interface_Engine/includes/UI_errorHandling.h"
#include "L2-Interface_Engine/includes/UI_colors.h"
#include "L2-Interface_Engine/includes/UI_disclaimer.h"
#include "L2-Interface_Engine/includes/UI_Engine.h"
#include "L2-Interface_Engine/includes/UI_input.h"
#include <curl/curl.h>
#include <csignal>          // signal(), SIGINT (POSIX)
using namespace std;
namespace fs = std::filesystem;

// ── Ctrl+C handler ──────────────────────────────────────────────────
// Sets g_interrupted flag; readInput() returns "quit" when getline()
// is interrupted, triggering normal shutdown through the quit path.
#ifdef _WIN32
static BOOL WINAPI ctrlHandler(DWORD dwCtrlType) {
    if (dwCtrlType == CTRL_C_EVENT || dwCtrlType == CTRL_BREAK_EVENT) {
        g_interrupted = 1;
        return TRUE;   // handled — don't pass to next handler
    }
    return FALSE;      // unhandled — pass to next handler
}
#else
extern "C" void handleSIGINT(int /*sig*/) {
    g_interrupted = 1;
}
#endif

int main(int argc, char* argv[]) {
    // ── parse CLI arguments first, before any init ──────────────
    ParsedArgs args = parseArgs(argc, argv);

    if (!args.error.empty()) {
        std::cerr << "Error: " << args.error << "\n\n";
        printUsage(std::cerr);
        return 1;
    }

    if (args.help) {
        printUsage(std::cout);
        return 0;
    }

    if (args.version) {
        printVersion(std::cout);
        return 0;
    }

    // ── cache-clear: handled after directory creation (line ~122) ───
    // ── ignore-config: handled as empty path in ConfigManager ctor ──
    // ── set/reset: handled after config load (line ~145) ────────────

    // ── set session flags from CLI args (override config behavior) ─
    g_quietMode   = args.quiet;
    g_verboseMode = args.verbose;
    g_offlineMode = args.offline;
    g_noBanner    = args.noBanner;

    // ── ensure curl_global_cleanup() is called on all exit paths ─
    struct CurlGuard { ~CurlGuard() { curl_global_cleanup(); } } curlGuard;

    // ── register Ctrl+C handler before any blocking I/O ────────────
#ifdef _WIN32
    SetConsoleCtrlHandler(ctrlHandler, TRUE);
#else
    signal(SIGINT, handleSIGINT);
#endif

    // ── register error callbacks before any L0 calls ─────────────────────
    // L0-core (DatabaseManager, UserDataManager, ConfigManager, PathResolver)
    // uses g_errorHandler to report errors. Must be set up first so that no
    // error is silently swallowed due to a null std::function member.
    g_errorHandler = { UI_fatal, UI_errors, UI_attention };

    // ── init libcurl before any L0 calls ─────────────────────────
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // ── create required directories ────────────────────────────────────────
    // creates user directories under ~/.config/tguide and ~/.local/share/tguide
    if (!PathResolver::createSystemDirs()) {
        UI_fatal("Failed to create required directories.\n"
                 "Check filesystem permissions.");
        return 1;
    }

    // non-fatal — user data dir is always writable, app runs without it
    if (!PathResolver::createUserDirs())
        UI_errors("Failed to create user data directories. "
                  "Saved commands and scripts may be unavailable.");

    // ── handle --cache-clear: wipe cached data and exit ──────────
    if (args.cacheClear) {
        std::string cachePath = PathResolver::cacheFile().string();
        if (!cachePath.empty() && std::filesystem::exists(cachePath)) {
            if (std::filesystem::remove(cachePath)) {
                std::cout << "Cache cleared." << std::endl;
            } else {
                std::cerr << "Error: Failed to clear cache." << std::endl;
                return 1;
            }
        } else {
            std::cout << "No cache to clear." << std::endl;
        }
        return 0;
    }

    // ── load config (or use factory defaults for --ignore-config) ─
    ConfigManager cfg(args.ignoreConfig
        ? std::string()
        : PathResolver::configFile().string());

    // ── init color toggle from config (--no-color overrides) ───────────────
    // must happen before any UI output so the correct mode is in effect
    bool colors = cfg.get<int>("colors", 1) == 1;
    if (args.noColor) colors = false;
    initColors(colors);

    // ── handle --set: modify a config setting and exit ───────────
    if (!args.setArg.empty()) {
        auto eqPos = args.setArg.find('=');
        if (eqPos == std::string::npos) {
            std::cerr << "Error: --set requires <key>=<value> format, "
                      << "e.g. --set colors=off" << std::endl;
            return 1;
        }
        std::string key = args.setArg.substr(0, eqPos);
        std::string val = args.setArg.substr(eqPos + 1);

        if (val == "on")        cfg.set<int>(key, 1);
        else if (val == "off")  cfg.set<int>(key, 0);
        else {
            try { cfg.set<int>(key, std::stoi(val)); }
            catch (...) { cfg.set<std::string>(key, val); }
        }

        if (cfg.save()) {
            std::cout << "Setting '" << key << "' set to '" << val << "'."
                      << std::endl;
        } else {
            std::cerr << "Error: Failed to write config." << std::endl;
            return 1;
        }
        return 0;
    }

    // ── handle --reset: reset setting(s) to defaults ─────────────
    if (!args.resetArg.empty()) {
        if (args.resetArg == "all") {
            // Delete config file and reload fresh defaults
            std::string configPath = PathResolver::configFile().string();
            if (!std::filesystem::remove(configPath)) {
                std::cerr << "Error: Failed to remove config file." << std::endl;
                return 1;
            }
            cfg = ConfigManager(configPath);  // fresh defaults → auto-saved
            std::cout << "All settings reset to defaults." << std::endl;
            return 0;
        } else {
            std::cerr << "Error: Resetting individual settings ('" << args.resetArg
                      << "') is not yet implemented. Use --reset all."
                      << std::endl;
            return 1;
        }
        return 0;
    }

    // ── init user data storage (singleton — survives bootstrap) ────────────
    // non-fatal: missing or unreadable files are recreated automatically
    UserDataManager::instance().init(
        PathResolver::savedCommandsFile().string(),
        PathResolver::savedScriptsFile().string()
    );
    if (!UserDataManager::instance().load())
        UI_errors("Failed to load user data files. Saved data may be unavailable.");

    // ── legal disclaimer — first run only ──────────────────────────────────
    if (cfg.get<int>("disclaimer_accepted", 0) == 0) {
        if (!UIDisclaimer::show(cfg))
            return 0;
    }

    // ── init cache before any DB class is constructed ──────────────────────
    // resolveDatabase() calls DBCache internally — must be ready first
    DBCacheManager::instance().init(PathResolver::cacheFile().string());

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

    // ── warn if DB is still missing after non-fatal resolve ────────────────
    // With STEP-B1b/B1c, the seed DB should always be available from the
    // install path. This check catches edge cases (corrupt install, dev
    // running from build tree without cmake --install).
    if (!std::filesystem::exists(PathResolver::dbFile())) {
        UI_errors("Database file is missing. Tool data will be unavailable.\n"
                  "Install tguide properly using 'cmake --install' or "
                  "check your internet connection for an update.");
    }

    // ── database integrity check ──────────────────────────────────────────
    // Runs PRAGMA integrity_check to detect corruption that may have passed
    // schema validation. If corruption is found, offers recovery from backup.
    // Guarded by existence check — sqlite3_open() would create an empty file
    // if the DB is absent (e.g. after non-fatal resolve failure).
    if (std::filesystem::exists(PathResolver::dbFile())) {
        sqlite3* db = nullptr;
        if (sqlite3_open(PathResolver::dbFile().string().c_str(), &db) == SQLITE_OK) {
            bool needsRecovery = false;

            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db, "PRAGMA integrity_check", -1, &stmt, nullptr) == SQLITE_OK) {
                while (sqlite3_step(stmt) == SQLITE_ROW) {
                    const char* text = reinterpret_cast<const char*>(
                        sqlite3_column_text(stmt, 0));
                    if (text && std::string(text) != "ok") {
                        needsRecovery = true;
                        break;
                    }
                }
                sqlite3_finalize(stmt);
            } else {
                // Prepare failure is itself strong evidence of corruption
                needsRecovery = true;
            }
            sqlite3_close(db);

            if (needsRecovery) {
                std::string bakPath = PathResolver::dbFile().string() + ".bak";
                bool backupExists = std::filesystem::exists(bakPath);

                while (true) {
                    cout << "\n"
                         << (colorsEnabled() ? Color::YELLOW : "")
                         << (colorsEnabled() ? Color::BOLD : "")
                         << "  [!] Database integrity check failed.\n"
                         << (colorsEnabled() ? Color::RESET : "")
                         << "\n"
                         << "  The database file may be corrupted.\n";

                    if (backupExists) {
                        cout << "  A backup is available.\n";
                    }

                    cout << "\n"
                         << "  [R] Restore from backup"
                         << (backupExists ? "" : " (no backup available)")
                         << "\n"
                         << "  [K] Keep current (may cause errors)\n"
                         << "  [A] Ask again on next launch\n"
                         << "\n";

                    string input = readInput("  \u2192 ");
                    if (input.empty()) continue;
                    if (isQuit(input)) { handleQuit(); return 0; }

                    char c = std::tolower(static_cast<unsigned char>(input[0]));

                    if (c == 'r' && backupExists) {
                        if (DBResolver::tryRestoreFromBackup(
                                bakPath, PathResolver::dbFile().string())) {
                            // Clear backup hash after restore
                            DBCacheManager::instance().clearBackup();
                            DBCacheManager::instance().save();
                            cout << "\n  Backup restored successfully.\n\n";
                        } else {
                            cout << "\n  Backup is corrupted or invalid. Cannot restore.\n\n";
                        }
                        waitForEnter();
                        break;
                    } else if (c == 'k' || c == 'a') {
                        break;
                    }
                }
            }
        }
    }

    // ── hand off to UI ─────────────────────────────────────────────────────
    UIEngine::start(cfg);

    return 0;
}