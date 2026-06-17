/*
 *  tguide — UserDataManager.h
 *  JSON-based storage for saved commands and generated scripts
 *
 *  written by voidoxin
 */

#pragma once
#include <string>
#include <vector>

// ==================== STRUCTS ====================

struct SavedCommand {
    int         id;
    int         tool_id;
    std::string command;   // full built command string
    std::string note;      // one-line description
};

struct SavedScript {
    int         id;
    std::string name;      // script filename without extension
    std::string path;      // absolute path to generated script file
    std::string note;      // one-line description
};

// ==================== UserDataManager ====================

/*
 * Manages saved_commands.json and saved_scripts.json.
 * No SQLite — both files live in userDataDir() and require no root.
 * Caller must call load() explicitly after construction.
 * save() writes both files; returns false if either write fails.
 */
class UserDataManager {
public:
    static UserDataManager& instance();
    void init(const std::string& commandsPath,
              const std::string& scriptsPath);

    UserDataManager(const std::string& commandsPath,
                    const std::string& scriptsPath);

    bool load();
    bool save();

    // returns new id on success, -1 on failure
    int  saveCommand(int tool_id, const std::string& command,
                     const std::string& note);
    bool updateCommand(int id, int tool_id, const std::string& command,
                       const std::string& note);
    bool deleteCommand(int id);
    std::vector<SavedCommand> getCommands();
    SavedCommand getCommandById(int id);

    // returns new id on success, -1 on failure
    int  saveScript(const std::string& name, const std::string& path,
                    const std::string& note);
    bool deleteScript(int id);
    std::vector<SavedScript> getScripts();

private:
    UserDataManager() = default;

    std::string               m_commandsPath;
    std::string               m_scriptsPath;
    std::vector<SavedCommand> m_commands;
    std::vector<SavedScript>  m_scripts;
    bool                      m_initialized = false;

    int nextCommandId() const;
    int nextScriptId()  const;
};