/*
 *  tguide — UI_colors.h
 *  written by voidoxin
 */

#pragma once

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <unistd.h>
#endif
#include <cstdio>
#include <atomic>

namespace Color {
    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* BLUE    = "\033[34m";
    constexpr const char* BOLD    = "\033[1m";
    constexpr const char* DIM     = "\033[2m";
    constexpr const char* RESET   = "\033[0m";
}

// ==================== RUNTIME COLOR TOGGLE ====================
//
// initColors() is called once at startup from CoreRunner after config loads.
// colorsEnabled() is used at every output site via the pattern:
//   cout << (colorsEnabled() ? Color::RED : "") << msg
//        << (colorsEnabled() ? Color::RESET : "");
//
// Windows: attempts to enable Virtual Terminal Processing via SetConsoleMode.
// Falls back to no colors on legacy consoles (cmd.exe without VT support).
// macOS, Linux, and Termux honor the user's color preference from config.

// returns true when stdout is a real terminal (not piped / redirected)
inline bool g_colorEnabled = false;

// ── Ctrl+C interrupt flag ────────────────────────────────────────
// Set by signal handler (SIGINT on POSIX, SetConsoleCtrlHandler on Windows).
// Checked by readInput() to gracefully exit when getline() is interrupted.
inline std::atomic<bool> g_interrupted{false};

inline bool isTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdout));
#endif
}

inline void initColors(bool enabled) {
#if defined(_WIN32)
    // Enable Virtual Terminal Processing on modern Windows Terminal
    // Falls back to no colors on legacy consoles (cmd.exe without VT support)
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode)) {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            if (SetConsoleMode(hOut, mode)) {
                g_colorEnabled = enabled;
            } else {
                g_colorEnabled = false;
            }
        } else {
            g_colorEnabled = false;
        }
    } else {
        g_colorEnabled = false;
    }
#else
    g_colorEnabled = enabled; // Linux, macOS, Termux: honor user preference
#endif
}

inline bool colorsEnabled() {
    return g_colorEnabled && isTerminal();
}