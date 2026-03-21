#pragma once
#include "../../L0-core/include/config_manager.h"

namespace UIDisclaimer {
    // returns true if accepted, false if user chose to exit
    bool show(ConfigManager& cfg);
}