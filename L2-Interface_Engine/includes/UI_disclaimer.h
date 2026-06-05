#pragma once

class ConfigManager;

namespace UIDisclaimer {
    // returns true if accepted, false if user chose to exit
    bool show(ConfigManager& cfg);
}