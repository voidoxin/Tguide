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


namespace fs = std::filesystem;

namespace PathResolver {

    // ── detect Termux ──────────────────────────────────────────────────────
    // Termux always exposes $PREFIX pointing to its internal usr directory
    static inline bool isTermux() {
        const char* prefix = getenv("PREFIX");
        if (!prefix) return false;
        return std::string(prefix).find("com.termux") != std::string::npos;
    }

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
        const char* home = getenv("HOME");
        if (home) return fs::path(home) / ".config/tguide";
        return fs::path(".");
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
        const char* home = getenv("HOME");
        if (home) return fs::path(home) / ".local/share/tguide";
        return fs::path(".");
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

    // ── bundled seed DB install path per platform ──────────────────────────
    // Returns the path where CMake / the package manager installs the bundled
    // seed database.  This is *system‑wide* (or OS‑managed) and is *distinct*
    // from dbFile() on every desktop platform so that copyDefaultToConfig() has a real
    // source → destination copy to perform on first boot.
    // Used by DBResolver::copyDefaultToConfig() at first-boot time.
    static inline fs::path installDbFile() {
#ifdef _WIN32
        // Windows: %PROGRAMDATA% = "C:\ProgramData" — system‑wide, all users
        const char* progdata = getenv("PROGRAMDATA");
        if (progdata) return fs::path(progdata) / "tguide/tguide.db";
        return fs::path(".");
#elif defined(__APPLE__)
        // macOS: /Library/Application Support — system Library, NOT ~/Library
        return fs::path("/Library/Application Support/tguide/tguide.db");
#else
        if (isTermux()) {
            // Termux: inside $PREFIX (always set in Termux)
            const char* prefix = getenv("PREFIX");
            if (prefix) return fs::path(prefix) / "share/tguide/tguide.db";
            return fs::path(".");   // consistent fallback
        }
        // Linux — system‑wide install path under /usr/local, distinct from user dbFile()
        return fs::path("/usr/local/share/tguide/tguide.db");
#endif
    }

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

} // namespace PathResolver
