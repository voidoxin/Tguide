/*
 *  tguide — UI_Engine.h
 *  written by voidoxin
 */

#pragma once
#include <string>
#include <vector>
#include <functional>

class ConfigManager;

namespace UIEngine {

    struct MenuItem {
        std::string icon;       // e.g.  "⚡", "◉", "⊞"
        std::string label;      // displayed name
        std::string hint;       // short description shown beside label
        std::function<void()> action;
    };

    // render a menu and block until user picks a valid option
    // returns the index chosen
    int renderMenu(
        const std::string&           section,
        const std::vector<MenuItem>& items
    );

    // main entry point — starts the UI loop
    void start(ConfigManager& cfg);

}