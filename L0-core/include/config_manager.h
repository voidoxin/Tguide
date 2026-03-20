#pragma once
#include <string>
#include "../../libs/json.hpp"

using json = nlohmann::json;

class ConfigManager {
private:
    std::string path;
    json config;

    void merge(json& target, const json& defaults);
    void clean(json& target, const json& defaults);

public:
    ConfigManager(const std::string& filepath);

    bool load();
    bool save() const;

    const json& get() const;
    template<typename T>
    T get(const std::string& keyPath, T defaultValue) const;

    template<typename T>
    void set(const std::string& keyPath, T value);
};