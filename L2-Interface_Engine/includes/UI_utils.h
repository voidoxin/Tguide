/*
 *  tguide — UI_utils.h
 *  written by voidoxin
 */

#pragma once
#include <string>

namespace UI {
    void clearScreen() noexcept;
    void printBanner();
    void printDivider();
    void printBreadcrumb(const std::string& section);
}