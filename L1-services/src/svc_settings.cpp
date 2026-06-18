/*
 *  tguide — svc_settings.cpp
 *  written by voidoxin
 *
 *  Service layer for the Settings screen.
 *  Connects UI_settings → L0-core ConfigManager.
 */

#include "../includes/svc_settings.h"
#include "../../L0-core/include/config_manager.h"

namespace SvcSettings {

bool getColorEnabled(const ConfigManager& cfg) {
    return cfg.get<int>("colors", 1) != 0;
}

bool setColorEnabled(ConfigManager& cfg, bool enabled) {
    cfg.set<int>("colors", enabled ? 1 : 0);
    return cfg.save();
}

} // namespace SvcSettings