#include "db_cache_manager.h"
#include "sha256.h"
#include <fstream>
#include <filesystem>

namespace DBCache {

static json        s_cache;
static std::string s_cachePath;

static constexpr int MAX_HISTORY = 3;

static json buildDefault() {
    return {
        {
            "_notice",
            "DO NOT TOUCH — managed automatically by the application. "
            "Manual edits will corrupt database integrity verification."
        },
        {
            "meta", {
                { "official_hash", DB_OFFICIAL_HASH },
                { "current_hash",  ""               },
                { "download_url",  DB_DOWNLOAD_URL  }
            }
        },
        { "history", json::array() }
    };
}

static void repairSchema() {
    json def = buildDefault();
    for (auto& [key, val] : def.items()) {
        if (!s_cache.contains(key))
            s_cache[key] = val;
    }
    for (auto& [key, val] : def["meta"].items()) {
        if (!s_cache["meta"].contains(key))
            s_cache["meta"][key] = val;
    }
}

void init(const std::string& cachePath) {
    s_cachePath = cachePath;

    std::filesystem::create_directories(
        std::filesystem::path(cachePath).parent_path()
    );

    if (!std::filesystem::exists(cachePath)) {
        s_cache = buildDefault();
        save(cachePath);
    } else {
        load(cachePath);
    }
}

bool load(const std::string& cachePath) {
    s_cachePath = cachePath;
    std::ifstream in(cachePath);
    if (!in.is_open()) return false;

    try {
        in >> s_cache;
    } catch (...) {
        s_cache = buildDefault();
        save(cachePath);
        return false;
    }

    repairSchema();
    return true;
}

bool save(const std::string& cachePath) {
    std::ofstream out(cachePath);
    if (!out.is_open()) return false;
    out << s_cache.dump(4);
    return true;
}

void recordAccess(const std::string& path, const std::string& hash) {
    auto& history = s_cache["history"];

    if (!history.empty()) {
        auto& last = history.back();
        if (last["path"] == path && last["hash"] == hash)
            return;
    }

    history.push_back({ {"path", path}, {"hash", hash} });

    while ((int)history.size() > MAX_HISTORY)
        history.erase(history.begin());
}

const std::vector<DBRecord>& getHistory() {
    static std::vector<DBRecord> result;
    result.clear();

    for (auto& entry : s_cache["history"]) {
        DBRecord r;
        r.path = entry.value("path", "");
        r.hash = entry.value("hash", "");
        result.push_back(r);
    }
    return result;
}

void setCurrentHash(const std::string& hash) {
    s_cache["meta"]["current_hash"] = hash;
}

const std::string& getCurrentHash() {
    static std::string result;
    result = s_cache["meta"].value("current_hash", "");
    return result;
}

bool isCurrentOfficial() {
    if (std::string(DB_OFFICIAL_HASH).empty()) return false;

    const std::string& current = getCurrentHash();
    if (current.empty()) return false;

    return current == DB_OFFICIAL_HASH;
}

std::optional<DBRecord> findOfficialInHistory() {
    if (std::string(DB_OFFICIAL_HASH).empty()) return std::nullopt;

    for (auto& record : getHistory()) {
        if (record.hash != DB_OFFICIAL_HASH) continue;
        if (!std::filesystem::exists(record.path)) continue;
        return record;
    }

    return std::nullopt;
}

} // namespace DBCache