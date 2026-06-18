/*
 *  tguide — UI_settings.cpp
 *  written by voidoxin
 *
 *  Settings screen — runtime configuration editor.
 *  Displays and modifies config values: color toggle, database management,
 *  and future settings.
 *  Connects upward to svc_settings → L0-core ConfigManager.
 */

#include "../includes/UI_settings.h"
#include "../includes/UI_utils.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_errorHandling.h"
#include "../includes/UI_input.h"
#include "../../L0-core/include/db_cache_manager.h"
#include "../../L0-core/include/path_resolver.h"
#include "../../L0-core/include/string_utils.h"
#include "../../L1-services/includes/svc_settings.h"
#include <filesystem>
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace std;

void UISettings::show(ConfigManager& cfg) {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("settings");
        UI::printDivider();

        bool colorOn = SvcSettings::getColorEnabled(cfg);

        cout << "\n"
             << (colorsEnabled() ? Color::BOLD : "")
             << "  Settings\n"
             << (colorsEnabled() ? Color::RESET : "");

        cout << "\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u251c\u2500 \u25c9  Color output"
             << (colorsEnabled() ? Color::RESET : "")
             << "              [1]\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2502    "
             << (colorsEnabled() ? Color::RESET : "")
             << (colorOn
                 ? (colorsEnabled() ? string(Color::GREEN) : "") + string("enabled")
                 : (colorsEnabled() ? string(Color::YELLOW) : "") + string("disabled"))
             << (colorsEnabled() ? Color::RESET : "")
             << "\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u251c\u2500 \u25c9  Database Management"
             << (colorsEnabled() ? Color::RESET : "")
             << "       [2]\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2502    "
             << (colorsEnabled() ? Color::RESET : "")
             << "manage backups and rollback\n"
             << (colorsEnabled() ? Color::DIM : "")
             << "  \u2514\u2500 "
             << (colorsEnabled() ? Color::RESET : "")
             << "\u2190  "
             << Strings::get(StringID::TOOLS_BACK)
             << "                         [0]\n";

        UI::printDivider();
        cout << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        if (input == "1") {
            bool newState = !colorOn;
            bool saved = SvcSettings::setColorEnabled(cfg, newState);
            initColors(newState);

            if (saved) {
                cout << "\n  "
                     << (colorsEnabled() ? Color::GREEN : "")
                     << "\u2713"
                     << (colorsEnabled() ? Color::RESET : "")
                     << "  color output "
                     << (newState ? "enabled" : "disabled")
                     << "\n\n";
            } else {
                cout << "\n  "
                     << (colorsEnabled() ? Color::YELLOW : "")
                     << "!  failed to persist preference (color "
                     << (newState ? "enabled" : "disabled")
                     << " for this session)"
                     << (colorsEnabled() ? Color::RESET : "")
                     << "\n\n";
            }

            waitForEnter();

        } else if (input == "2") {
            showDatabaseMenu();

        } else {
            cout << (colorsEnabled() ? Color::YELLOW : "")
                 << "\n  invalid choice \u2014 try again.\n\n"
                 << (colorsEnabled() ? Color::RESET : "");
            waitForEnter();
        }
    }
}

void UISettings::showDatabaseMenu() {
    while (true) {
        UI::clearScreen();
        UI::printBanner();
        UI::printBreadcrumb("settings \u203a database");
        UI::printDivider();

        // Fetch fresh DB info each loop
        SvcSettings::DbInfo info = SvcSettings::getDbInfo();

        // Format file size
        std::string fileSizeStr;
        if (info.fileSize >= 1048576) {
            double mb = static_cast<double>(info.fileSize) / 1048576.0;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << mb << " MB";
            fileSizeStr = oss.str();
        } else if (info.fileSize >= 1024) {
            double kb = static_cast<double>(info.fileSize) / 1024.0;
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(1) << kb << " KB";
            fileSizeStr = oss.str();
        } else {
            fileSizeStr = std::to_string(info.fileSize) + " bytes";
        }

        cout << "\n"
             << (colorsEnabled() ? Color::BOLD : "")
             << "  Database Management\n"
             << (colorsEnabled() ? Color::RESET : "")
             << "\n";

        // ── DB Info section ──
        cout << (colorsEnabled() ? Color::DIM : "") << "  Active version"
             << (colorsEnabled() ? Color::RESET : "")
             << "  " << (info.version.empty() ? "unknown" : info.version) << "\n";

        cout << (colorsEnabled() ? Color::DIM : "") << "  File size"
             << (colorsEnabled() ? Color::RESET : "")
             << "        " << fileSizeStr << "\n";

        cout << (colorsEnabled() ? Color::DIM : "") << "  Tables"
             << (colorsEnabled() ? Color::RESET : "")
             << "            " << info.tableCount << "\n";

        cout << (colorsEnabled() ? Color::DIM : "") << "  Total rows"
             << (colorsEnabled() ? Color::RESET : "")
             << "        " << info.rowCount << "\n";

        // ── Backup section ──
        cout << (colorsEnabled() ? Color::DIM : "") << "  Backup"
             << (colorsEnabled() ? Color::RESET : "")
             << "          " << (info.hasBackup ? "available" : "none") << "\n";

        if (info.hasBackup) {
            string backupHash = DBCacheManager::instance().getBackupHash();
            if (!backupHash.empty()) {
                cout << (colorsEnabled() ? Color::DIM : "") << "  Backup hash"
                     << (colorsEnabled() ? Color::RESET : "")
                     << "    " << backupHash.substr(0, 16) << "...\n";
            }
        }

        cout << "\n";

        // ── Actions ──
        cout << "  [U] Update Database \u2014 check GitHub for latest version\n";

        if (info.hasBackup) {
            cout << "  [R] Rollback Database \u2014 replace current DB with backup\n"
                 << "  [D] Delete Backup\n";
        } else {
            cout << (colorsEnabled() ? Color::DIM : "")
                 << "  No backup available. Backups are created automatically\n"
                 << "  before database updates.\n"
                 << (colorsEnabled() ? Color::RESET : "");
        }

        cout << "\n";

        UI::printDivider();
        cout << "\n";

        string input = readInput("  \u2192 ");
        if (input.empty()) continue;
        if (isQuit(input)) { handleQuit(); return; }
        if (isBack(input)) return;

        char c = std::tolower(static_cast<unsigned char>(input[0]));

        if (c == 'u') {
            bool success = SvcSettings::triggerDbUpdate();
            if (success) {
                SvcSettings::DbInfo updatedInfo = SvcSettings::getDbInfo();
                cout << "\n  "
                     << (colorsEnabled() ? Color::GREEN : "")
                     << "\u2713  Database updated to version "
                     << (updatedInfo.version.empty() ? "unknown" : updatedInfo.version)
                     << (colorsEnabled() ? Color::RESET : "")
                     << "\n\n";
            } else {
                cout << "\n  "
                     << (colorsEnabled() ? Color::YELLOW : "")
                     << "!  Update failed \u2014 check internet connection"
                     << (colorsEnabled() ? Color::RESET : "")
                     << "\n\n";
            }
            waitForEnter();

        } else if (c == 'r' && info.hasBackup) {
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

        } else if (c == 'd' && info.hasBackup) {
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

        } else {
            cout << (colorsEnabled() ? Color::YELLOW : "")
                 << "\n  invalid choice.\n\n"
                 << (colorsEnabled() ? Color::RESET : "");
            waitForEnter();
        }
    }
}