/*
 *  tguide — path_resolver.h
 *  cross-platform path resolution
 *
 *  written by voidoxin
 */

#pragma once
#include <filesystem>
#include <cstdlib>
#include <string>

#ifndef _WIN32
#include <unistd.h>   // geteuid()
#endif

namespace fs = std::filesystem;

namespace PathResolver {

    // ── detect Termux ──────────────────────────────────────────────────────
    // Termux always exposes $PREFIX pointing to its internal usr directory
    static inline bool isTermux() {
        const char* prefix = getenv("PREFIX");
        if (!prefix) return false;
        return std::string(prefix).find("com.termux") != std::string::npos;
    }

    // ── detect if running as root ──────────────────────────────────────────
#ifndef _WIN32
    static inline bool isRoot() {
        return geteuid() == 0;
    }
#endif

    // ── config path ────────────────────────────────────────────────────────
    static inline fs::path configDir() {
#ifdef _WIN32
        const char* appdata = getenv("APPDATA");
        if (appdata) return fs::path(appdata) / "tguide";
        return fs::path(".");
#elif defined(__APPLE__)
        const char* home = getenv("HOME");
        if (home) return fs::path(home) / "Library/Application Support/tguide";
        return fs::path(".");
#else
        if (isTermux()) {
            const char* prefix = getenv("PREFIX");
            if (prefix) return fs::path(prefix) / "etc/tguide";
        }
        // Linux — needs root to write here
        return fs::path("/etc/tguide");
#endif
    }

    // ── data path ──────────────────────────────────────────────────────────
    static inline fs::path dataDir() {
#ifdef _WIN32
        const char* appdata = getenv("APPDATA");
        if (appdata) return fs::path(appdata) / "tguide";
        return fs::path(".");
#elif defined(__APPLE__)
        const char* home = getenv("HOME");
        if (home) return fs::path(home) / "Library/Application Support/tguide";
        return fs::path(".");
#else
        if (isTermux()) {
            const char* prefix = getenv("PREFIX");
            if (prefix) return fs::path(prefix) / "share/tguide";
        }
        // Linux — needs root to write here
        return fs::path("/usr/share/tguide");
#endif
    }

    // ── user data path — writable without root on every platform ──────────
    // distinct from dataDir() on Linux: ~/.local/share instead of /usr/share
    static inline fs::path userDataDir() {
#ifdef _WIN32
        const char* appdata = getenv("APPDATA");
        if (appdata) return fs::path(appdata) / "tguide";
        return fs::path(".");
#elif defined(__APPLE__)
        const char* home = getenv("HOME");
        if (home) return fs::path(home) / "Library/Application Support/tguide";
        return fs::path(".");
#else
        if (isTermux()) {
            const char* prefix = getenv("PREFIX");
            if (prefix) return fs::path(prefix) / "share/tguide";
        }
        // Linux — user-writable without root
        const char* home = getenv("HOME");
        if (home) return fs::path(home) / ".local/share/tguide";
        return fs::path(".");
#endif
    }

    // ── backup path ────────────────────────────────────────────────────────
    // dev-only: removed before release
    static inline fs::path backupDir() {
        return dataDir() / "backup";
    }

    // ── resolved file paths ────────────────────────────────────────────────
    static inline fs::path configFile()        { return configDir()   / "config.json";         }
    static inline fs::path dbFile()            { return dataDir()     / "tguide.db";            }
    static inline fs::path backupFile()        { return backupDir()   / "tguide.db";            }
    static inline fs::path cacheFile()         { return dataDir()     / ".db_cache";            }
    static inline fs::path savedCommandsFile() { return userDataDir() / "saved_commands.json";  }
    static inline fs::path savedScriptsFile()  { return userDataDir() / "saved_scripts.json";   }
    static inline fs::path scriptsDir()        { return userDataDir() / "scripts";              }

    // ── create system directories — fatal if these fail ───────────────────
    // configDir + dataDir + backupDir: root dirs on Linux, always need to exist
    static inline bool createSystemDirs() {
        std::error_code ec;
        fs::create_directories(configDir(), ec);
        if (ec) return false;
        fs::create_directories(dataDir(), ec);
        if (ec) return false;
        fs::create_directories(backupDir(), ec);
        if (ec) return false;
        return true;
    }

    // ── create user data directories — non-fatal ──────────────────────────
    // userDataDir + scriptsDir: always writable without root
    // caller uses UI_errors on false — program continues
    static inline bool createUserDirs() {
        std::error_code ec;
        fs::create_directories(userDataDir(), ec);
        if (ec) return false;
        fs::create_directories(scriptsDir(), ec);
        if (ec) return false;
        return true;
    }

    // ── create required directories — kept for backward compatibility ──────
    // calls createSystemDirs() only; user dirs handled separately in CoreRunner
    static inline bool createDirs() {
        return createSystemDirs();
    }

    // ── check if we have write access to required dirs ─────────────────────
    // used on Linux to warn the user before anything fails
    static inline bool hasWriteAccess() {
#ifdef _WIN32
        return true;          // APPDATA is always writable
#elif defined(__APPLE__)
        return true;          // ~/Library/Application Support/ is always writable
#else
        if (isTermux()) return true;   // PREFIX is always writable
        return isRoot();
#endif
    }

} // namespace PathResolver
