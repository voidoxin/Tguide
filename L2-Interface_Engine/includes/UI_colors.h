/*
 *  tguide — UI_colors.h
 *  written by voidoxin
 */

#pragma once

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <cstdio>

namespace Color {
#if defined(_WIN32) || defined(__APPLE__)
    constexpr const char* RED     = "";
    constexpr const char* GREEN   = "";
    constexpr const char* YELLOW  = "";
    constexpr const char* CYAN    = "";
    constexpr const char* MAGENTA = "";
    constexpr const char* BLUE    = "";
    constexpr const char* BOLD    = "";
    constexpr const char* DIM     = "";
    constexpr const char* RESET   = "";
#else
    constexpr const char* RED     = "\033[31m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* BLUE    = "\033[34m";
    constexpr const char* BOLD    = "\033[1m";
    constexpr const char* DIM     = "\033[2m";
    constexpr const char* RESET   = "\033[0m";
#endif
}

// ==================== RUNTIME COLOR TOGGLE ====================
//
// initColors() is called once at startup from CoreRunner after config loads.
// colorsEnabled() is used at every output site via the pattern:
//   cout << (colorsEnabled() ? Color::RED : "") << msg
//        << (colorsEnabled() ? Color::RESET : "");
//
// Windows and macOS always return false — no ANSI support assumed.
// Linux and Termux honor the value passed to initColors().

// returns true when stdout is a real terminal (not piped / redirected)
inline bool g_colorEnabled = false;

inline bool isTerminal() {
#ifdef _WIN32
    return _isatty(_fileno(stdout));
#else
    return isatty(fileno(stdout));
#endif
}

inline void initColors(bool enabled) {
#if defined(_WIN32) || defined(__APPLE__)
    (void)enabled;          // forced off on Windows and macOS
    g_colorEnabled = false;
#else
    g_colorEnabled = enabled;
#endif
}

inline bool colorsEnabled() {
    return g_colorEnabled && isTerminal();
}