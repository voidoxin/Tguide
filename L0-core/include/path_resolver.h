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
#else
        if (isTermux()) {
            const char* prefix = getenv("PREFIX");
            if (prefix) return fs::path(prefix) / "share/tguide";
        }
        // Linux — needs root to write here
        return fs::path("/usr/share/tguide");
#endif
    }

    // ── backup path ────────────────────────────────────────────────────────
    // dev-only: removed before release
    static inline fs::path backupDir() {
        return dataDir() / "backup";
    }

    // ── resolved file paths ────────────────────────────────────────────────
    static inline fs::path configFile() { return configDir() / "config.json"; }
    static inline fs::path dbFile()     { return dataDir()   / "tguide.db";   }
    static inline fs::path backupFile() { return backupDir() / "tguide.db";   }

    // ── create required directories ────────────────────────────────────────
    // on Linux this will fail silently if not root — caller handles the error
    static inline bool createDirs() {
        std::error_code ec;
        fs::create_directories(configDir(), ec);
        if (ec) return false;
        fs::create_directories(dataDir(), ec);
        if (ec) return false;
        fs::create_directories(backupDir(), ec);
        if (ec) return false;
        return true;
    }

    // ── check if we have write access to required dirs ─────────────────────
    // used on Linux to warn the user before anything fails
    static inline bool hasWriteAccess() {
#ifdef _WIN32
        return true; // APPDATA is always writable
#else
        if (isTermux()) return true; // PREFIX is always writable
        return isRoot();
#endif
    }

}