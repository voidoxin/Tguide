/*
 *  tguide — UI_settings.cpp
 *  written by voidoxin
 *
 *  Settings screen — runtime configuration editor.
 *  Future implementation will display current config values loaded from
 *  ConfigManager and allow the user to edit: theme, default platform,
 *  auto-update toggle, and database/config path overrides.
 *  Connects upward to svc_settings → L0-core ConfigManager once
 *  L2-services is implemented.
 */

#include "../includes/UI_settings.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include "../../L0-core/include/db_cache_manager.h"
#include "../../L0-core/include/path_resolver.h"
#include <filesystem>
#include <iostream>

using namespace std;

void UISettings::show() {
    UI::clearScreen();
    UI::printBanner();
    UI::printBreadcrumb("settings");
    UI::printDivider();

    cout << "\n"
         << Color::DIM << "  settings" << Color::RESET
         << Color::CYAN << " — " << Color::RESET
         << Color::BOLD << "coming soon" << Color::RESET
         << "\n\n"
         << Color::DIM
         << "  configure theme, platform defaults, and update behavior.\n"
         << "  this screen is not yet implemented.\n"
         << Color::RESET
         << "\n";

    UI::printDivider();
    cout << "\n";

    waitForEnter();
}

void UISettings::showDatabaseMenu() {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("settings \u203a database");
        UI::printDivider();

        string activeVersion = DBCacheManager::instance().getLastSeenVersion();
        bool hasBackup = DBCacheManager::instance().hasBackup();

        cout << "\n"
             << Color::BOLD << "  Database Management\n" << Color::RESET
             << "\n"
             << Color::DIM << "  Active version" << Color::RESET
             << "  " << (activeVersion.empty() ? "unknown" : activeVersion) << "\n"
             << Color::DIM << "  Backup" << Color::RESET
             << "          " << (hasBackup ? "available" : "none") << "\n";

        if (hasBackup) {
            string backupHash = DBCacheManager::instance().getBackupHash();
            if (!backupHash.empty()) {
                cout << Color::DIM << "  Backup hash" << Color::RESET
                     << "    " << backupHash.substr(0, 16) << "...\n";
            }
        }

        cout << "\n";

        if (hasBackup) {
            cout << "  [R] Rollback Database \u2014 replace current DB with backup\n"
                 << "  [D] Delete Backup\n"
                 << "\n";
        } else {
            cout << Color::DIM
                 << "  No backup available. Backups are created automatically\n"
                 << "  before database updates.\n"
                 << Color::RESET
                 << "\n";
        }

        UI::printDivider();
        cout << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        char c = std::tolower(static_cast<unsigned char>(input[0]));

        if (c == 'r' && hasBackup) {
            string bakPath = PathResolver::dbFile().string() + ".bak";
            std::error_code ec;

            if (!std::filesystem::exists(bakPath)) {
                cout << "\n  Backup file not found on disk.\n\n";
                waitForEnter();
                continue;
            }

            std::filesystem::copy_file(bakPath, PathResolver::dbFile().string(),
                std::filesystem::copy_options::overwrite_existing, ec);

            if (!ec) {
                DBCacheManager::instance().clearBackup();
                DBCacheManager::instance().save();
                cout << "\n  Database rolled back to backup version.\n\n";
            } else {
                cout << "\n  Failed to rollback database.\n\n";
            }
            waitForEnter();

        } else if (c == 'd' && hasBackup) {
            string bakPath = PathResolver::dbFile().string() + ".bak";
            std::error_code ec;
            std::filesystem::remove(bakPath, ec);

            if (!ec) {
                DBCacheManager::instance().clearBackup();
                DBCacheManager::instance().save();
                cout << "\n  Backup deleted.\n\n";
            } else {
                cout << "\n  Failed to delete backup.\n\n";
            }
            waitForEnter();
        }
    }
}