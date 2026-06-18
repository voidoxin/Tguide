#include "db_cache_manager.h"
#include "sha256.h"
#include <fstream>
#include <filesystem>

DBCacheManager& DBCacheManager::instance() {
    static DBCacheManager m;
    return m;
}

json DBCacheManager::buildDefault() {
    return {
        {
            "_notice",
            "DO NOT TOUCH — managed automatically by the application. "
            "Manual edits will corrupt database integrity verification."
        },
        {
            "meta", {
                { "current_hash", "" },
                { "last_version", "" },
                { "backup_hash", "" },
                { "pending_update", false }
            }
        },
        { "history", json::array() }
    };
}

void DBCacheManager::repairSchema() {
    json def = buildDefault();
    for (auto& [key, val] : def.items()) {
        if (!cache_.contains(key))
            cache_[key] = val;
    }
    for (auto& [key, val] : def["meta"].items()) {
        if (!cache_["meta"].contains(key))
            cache_["meta"][key] = val;
    }
}

void DBCacheManager::init(const std::string& cachePath) {
    cachePath_ = cachePath;

    std::filesystem::create_directories(
        std::filesystem::path(cachePath).parent_path()
    );

    if (!std::filesystem::exists(cachePath)) {
        cache_ = buildDefault();
        save(cachePath);
    } else {
        load(cachePath);
    }
}

bool DBCacheManager::load(const std::string& cachePath) {
    cachePath_ = cachePath;
    std::ifstream in(cachePath);
    if (!in.is_open()) return false;

    try {
        in >> cache_;
    } catch (...) {
        cache_ = buildDefault();
        save(cachePath);
        return false;
    }

    repairSchema();
    return true;
}

#ifndef NDEBUG
void DBCacheManager::resetForTesting() {
    cache_ = buildDefault();
    cachePath_.clear();
}
#endif

// save to internal cachePath_ — safe to call from resolveDatabase
bool DBCacheManager::save() {
    if (cachePath_.empty()) return false;
    return save(cachePath_);
}

bool DBCacheManager::save(const std::string& cachePath) {
    std::ofstream out(cachePath);
    if (!out.is_open()) return false;
    out << cache_.dump(4);
    return true;
}

void DBCacheManager::recordAccess(const std::string& path, const std::string& hash) {
    auto& history = cache_["history"];

    if (!history.empty()) {
        auto& last = history.back();
        if (last["path"] == path && last["hash"] == hash)
            return;
    }

    history.push_back({ {"path", path}, {"hash", hash} });

    while ((int)history.size() > MAX_HISTORY_)
        history.erase(history.begin());
}

// returns by value — safe to store across calls
std::vector<DBCacheManager::DBRecord> DBCacheManager::getHistory() {
    std::vector<DBRecord> result;

    for (auto& entry : cache_["history"]) {
        DBRecord r;
        r.path = entry.value("path", "");
        r.hash = entry.value("hash", "");
        result.push_back(r);
    }
    return result;
}

void DBCacheManager::setCurrentHash(const std::string& hash) {
    cache_["meta"]["current_hash"] = hash;
}

// returns by value — safe to store across calls
std::string DBCacheManager::getCurrentHash() {
    return cache_["meta"].value("current_hash", "");
}

void DBCacheManager::setLastSeenVersion(const std::string& version) {
    cache_["meta"]["last_version"] = version;
}

std::string DBCacheManager::getLastSeenVersion() {
    return cache_["meta"].value("last_version", "");
}

void DBCacheManager::setBackupHash(const std::string& hash) {
    cache_["meta"]["backup_hash"] = hash;
}

std::string DBCacheManager::getBackupHash() {
    return cache_["meta"].value("backup_hash", "");
}

bool DBCacheManager::hasBackup() {
    return !getBackupHash().empty();
}

void DBCacheManager::clearBackup() {
    cache_["meta"]["backup_hash"] = "";
}

void DBCacheManager::setPendingUpdate(bool pending) {
    cache_["meta"]["pending_update"] = pending;
}

bool DBCacheManager::hasPendingUpdate() {
    return cache_["meta"].value("pending_update", false);
}
