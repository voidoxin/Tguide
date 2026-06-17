#include "../include/config_manager.h"
#include <fstream>

ConfigManager::ConfigManager(const std::string& filepath) : path(filepath) {
    load();
}

void ConfigManager::merge(json& target, const json& defaults) {
    for (auto& [key, value] : defaults.items()) {
        if (!target.contains(key)
            || target[key].is_null()
            || (target[key].is_string() && target[key].get<std::string>().empty())) {
            target[key] = value;
        } else if (value.is_object()) {
            merge(target[key], value);
        }
    }
}

void ConfigManager::clean(json& target, const json& defaults) {
    for (auto it = target.begin(); it != target.end(); ) {
        if (!defaults.contains(it.key())) {
            it = target.erase(it);
        } else {
            if (it->is_object())
                clean(*it, defaults[it.key()]);
            ++it;
        }
    }
}

bool ConfigManager::load() {
    std::ifstream in(path);

    if (in.is_open()) {
        try {
            in >> config;
        } catch (...) {
            config = json::object();
        }
    }

    if (config.is_null())
        config = json::object();

    json defaultConfig = {
        {"App", {
            {"database", {
                {"db_path",     "data/database/tguide.db"},
                {"backup_path", "data/backup/tguide_bkp.db"},
                {"cache_path",  "data/database/.db_cache"}
            }},
            {"info", {
                {"version", "V1.0.1"}
            }}
        }},
        {"environment", {
            {"os",   "-1"},
            {"root", "unknown"}
        }},
        {"colors",              1},   // 1 = ANSI on, 0 = plain output
        {"disclaimer_accepted", 0}    // 0 = not accepted, 1 = accepted
    };

    json before = config;

    // Merge defaults into config so every expected key exists.
    // NOTE: clean() is intentionally NOT called here so that
    // programmatic keys set via ConfigManager::set() survive
    // reload. The old clean() call stripped non-default keys,
    // which broke save→load round-trips for custom values.
    merge(config, defaultConfig);

    if (config != before)
        return save();

    return true;
}

bool ConfigManager::save() const {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << config.dump(4);
    return true;
}

const json& ConfigManager::get() const {
    return config;
}

template<typename T>
T ConfigManager::get(const std::string& keyPath, T defaultValue) const {
    const json* ptr = &config;
    size_t start = 0, end;
    while ((end = keyPath.find('.', start)) != std::string::npos) {
        std::string key = keyPath.substr(start, end - start);
        if (!ptr->contains(key)) return defaultValue;
        ptr = &(*ptr)[key];
        start = end + 1;
    }
    std::string lastKey = keyPath.substr(start);
    if (!ptr->contains(lastKey)) return defaultValue;
    try {
        return ptr->at(lastKey).get<T>();
    } catch (...) {
        return defaultValue;
    }
}

template<typename T>
void ConfigManager::set(const std::string& keyPath, T value) {
    json* ptr = &config;
    size_t start = 0, end;
    while ((end = keyPath.find('.', start)) != std::string::npos) {
        std::string key = keyPath.substr(start, end - start);
        if (!ptr->contains(key)) (*ptr)[key] = json::object();
        ptr = &(*ptr)[key];
        start = end + 1;
    }
    (*ptr)[keyPath.substr(start)] = value;
}

template void ConfigManager::set<int>(const std::string&, int);
template void ConfigManager::set<bool>(const std::string&, bool);
template void ConfigManager::set<double>(const std::string&, double);
template void ConfigManager::set<std::string>(const std::string&, std::string);

template int ConfigManager::get<int>(const std::string&, int) const;
template bool ConfigManager::get<bool>(const std::string&, bool) const;
template double ConfigManager::get<double>(const std::string&, double) const;
template std::string ConfigManager::get<std::string>(const std::string&, std::string) const;