/*
 *  tguide — UserDataManager.cpp
 *  written by voidoxin
 */

#include "UserDataManager.h"
#include "ErrorHandler.h"
#include "../../libs/json.hpp"
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

// ==================== HELPERS ====================

static bool writeJson(const std::string& path, const json& data) {
    std::ofstream out(path);
    if (!out.is_open()) return false;
    out << data.dump(4);
    if (!out.good()) {
        if (g_errorHandler.error) g_errorHandler.error("UserDataManager: write failed for: " + path);
        return false;
    }
    return true;
}

static json readJson(const std::string& path) {
    std::ifstream in(path);
    if (!in.is_open()) return json::object();
    try {
        json data;
        in >> data;
        return data;
    } catch (...) {
        return json::object();
    }
}

// ==================== UserDataManager ====================

UserDataManager& UserDataManager::instance() {
    static UserDataManager m;
    return m;
}

void UserDataManager::init(const std::string& commandsPath,
                            const std::string& scriptsPath) {
    m_commandsPath = commandsPath;
    m_scriptsPath  = scriptsPath;
    m_initialized  = true;
}

UserDataManager::UserDataManager(const std::string& commandsPath,
                                 const std::string& scriptsPath)
    : m_commandsPath(commandsPath), m_scriptsPath(scriptsPath)
{
    m_initialized = true;
    // paths stored — caller must call load() explicitly
}

bool UserDataManager::load() {
    if (!m_initialized) return false;
    bool ok = true;

    // ── commands ──────────────────────────────────────────────────────────
    {
        json data = readJson(m_commandsPath);
        m_commands.clear();

        // empty file or missing root key — treat as first run, auto-create
        if (!data.contains("commands") || !data["commands"].is_array()) {
            if (!writeJson(m_commandsPath, { {"commands", json::array()} }))
                ok = false;
        } else {
            for (auto& item : data["commands"]) {
                try {
                    SavedCommand c;
                    c.id      = item.at("id").get<int>();
                    c.tool_id = item.at("tool_id").get<int>();
                    c.command = item.at("command").get<std::string>();
                    c.note    = item.at("note").get<std::string>();
                    m_commands.push_back(c);
                } catch (...) {
                    if (g_errorHandler.error) g_errorHandler.error("UserDataManager: skipped malformed command entry.");
                }
            }
        }
    }

    // ── scripts ───────────────────────────────────────────────────────────
    {
        json data = readJson(m_scriptsPath);
        m_scripts.clear();

        // empty file or missing root key — treat as first run, auto-create
        if (!data.contains("scripts") || !data["scripts"].is_array()) {
            if (!writeJson(m_scriptsPath, { {"scripts", json::array()} }))
                ok = false;
        } else {
            for (auto& item : data["scripts"]) {
                try {
                    SavedScript s;
                    s.id   = item.at("id").get<int>();
                    s.name = item.at("name").get<std::string>();
                    s.path = item.at("path").get<std::string>();
                    s.note = item.at("note").get<std::string>();
                    m_scripts.push_back(s);
                } catch (...) {
                    if (g_errorHandler.error) g_errorHandler.error("UserDataManager: skipped malformed script entry.");
                }
            }
        }
    }

    return ok;
}

bool UserDataManager::save() {
    if (!m_initialized) return false;
    json cmdArray = json::array();
    for (auto& c : m_commands) {
        cmdArray.push_back({
            {"id",      c.id},
            {"tool_id", c.tool_id},
            {"command", c.command},
            {"note",    c.note}
        });
    }

    json scrArray = json::array();
    for (auto& s : m_scripts) {
        scrArray.push_back({
            {"id",   s.id},
            {"name", s.name},
            {"path", s.path},
            {"note", s.note}
        });
    }

    // writeJson already calls UI_errors on write failure
    bool ok = writeJson(m_commandsPath, { {"commands", cmdArray} });
    ok     &= writeJson(m_scriptsPath,  { {"scripts",  scrArray} });

    return ok;
}

// ── id helpers ─────────────────────────────────────────────────────────────

int UserDataManager::nextCommandId() const {
    if (m_commands.empty()) return 1;
    int maxId = 0;
    for (auto& c : m_commands)
        if (c.id > maxId) maxId = c.id;
    return maxId + 1;
}

int UserDataManager::nextScriptId() const {
    if (m_scripts.empty()) return 1;
    int maxId = 0;
    for (auto& s : m_scripts)
        if (s.id > maxId) maxId = s.id;
    return maxId + 1;
}

// ── commands ───────────────────────────────────────────────────────────────

int UserDataManager::saveCommand(int tool_id, const std::string& command,
                                 const std::string& note) {
    if (!m_initialized) return -1;
    // empty path means user data dir was unavailable — fail immediately
    if (m_commandsPath.empty()) return -1;

    SavedCommand c;
    c.id      = nextCommandId();
    c.tool_id = tool_id;
    c.command = command;
    c.note    = note;
    m_commands.push_back(c);

    if (!save()) {
        m_commands.pop_back();
        return -1;
    }
    return c.id;
}

bool UserDataManager::updateCommand(int id, int tool_id,
                                     const std::string& command,
                                     const std::string& note) {
    if (!m_initialized) return false;
    auto it = std::find_if(m_commands.begin(), m_commands.end(),
        [id](const SavedCommand& c) { return c.id == id; });
    if (it == m_commands.end()) return false;

    SavedCommand backup = *it;
    it->tool_id = tool_id;
    it->command = command;
    it->note    = note;

    if (!save()) {
        *it = backup;
        return false;
    }
    return true;
}

SavedCommand UserDataManager::getCommandById(int id) {
    auto it = std::find_if(m_commands.begin(), m_commands.end(),
        [id](const SavedCommand& c) { return c.id == id; });
    if (it != m_commands.end()) return *it;
    return SavedCommand{};
}

bool UserDataManager::deleteCommand(int id) {
    if (!m_initialized) return false;
    auto it = std::find_if(m_commands.begin(), m_commands.end(),
        [id](const SavedCommand& c) { return c.id == id; });
    // id not found — silent false, no UI_errors (caller handles display)
    if (it == m_commands.end()) return false;

    SavedCommand backup = *it;
    m_commands.erase(it);

    if (!save()) {
        m_commands.push_back(backup);
        return false;
    }
    return true;
}

std::vector<SavedCommand> UserDataManager::getCommands() {
    return m_commands;
}

// ── scripts ────────────────────────────────────────────────────────────────

int UserDataManager::saveScript(const std::string& name, const std::string& path,
                                const std::string& note) {
    if (!m_initialized) return -1;
    // empty path means user data dir was unavailable — fail immediately
    if (m_scriptsPath.empty()) return -1;

    SavedScript s;
    s.id   = nextScriptId();
    s.name = name;
    s.path = path;
    s.note = note;
    m_scripts.push_back(s);

    if (!save()) {
        m_scripts.pop_back();
        return -1;
    }
    return s.id;
}

bool UserDataManager::updateScript(int id, const std::string& name,
                                    const std::string& path,
                                    const std::string& note) {
    if (!m_initialized) return false;
    auto it = std::find_if(m_scripts.begin(), m_scripts.end(),
        [id](const SavedScript& s) { return s.id == id; });
    if (it == m_scripts.end()) return false;

    SavedScript backup = *it;
    it->name = name;
    it->path = path;
    it->note = note;

    if (!save()) {
        *it = backup;
        return false;
    }
    return true;
}

SavedScript UserDataManager::getScriptById(int id) {
    auto it = std::find_if(m_scripts.begin(), m_scripts.end(),
        [id](const SavedScript& s) { return s.id == id; });
    if (it != m_scripts.end()) return *it;
    return SavedScript{};
}

bool UserDataManager::deleteScript(int id) {
    if (!m_initialized) return false;
    auto it = std::find_if(m_scripts.begin(), m_scripts.end(),
        [id](const SavedScript& s) { return s.id == id; });
    // id not found — silent false, no UI_errors (caller handles display)
    if (it == m_scripts.end()) return false;

    SavedScript backup = *it;
    m_scripts.erase(it);

    if (!save()) {
        m_scripts.push_back(backup);
        return false;
    }
    return true;
}

std::vector<SavedScript> UserDataManager::getScripts() {
    return m_scripts;
}