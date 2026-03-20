/*
 *  tguide — UI_colors.h
 *  written by voidoxin
 */

#pragma once

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