/*
 *  tguide — svc_savedCommands.cpp
 *  written by voidoxin
 *
 *  Service layer for the Saved Commands screen.
 *  Connects UI_savedCommands → L0-core user command storage.
 */

#include <algorithm>
#include <string>
#include <vector>

#include "../../L0-core/include/DatabaseManager.h"
#include "../../L0-core/include/path_resolver.h"
#include "../../L0-core/include/UserDataManager.h"
#include "../includes/svc_savedCommands.h"

using namespace std;

// ── L0→DTO mapper ────────────────────────────────────────────────────

static SvcDTO::SavedCommandDTO toDTO(const SavedCommand& c,
                                      const string& tool_name) {
    return { c.id, c.tool_id, c.command, c.note, tool_name };
}

// ── Resolve tool name from tool_id (empty string if not found) ───────

static string resolveToolName(int tool_id) {
    if (tool_id <= 0) return {};
    ToolD db(PathResolver::dbFile().string());
    ToolResults res = db.getWhere({"id"}, {to_string(tool_id)});
    if (res.items.empty()) return {};
    return res.items[0].name;
}

// ==================== NAMESPACE ====================

namespace SvcSavedCommands {

vector<SvcDTO::SavedCommandDTO> getAllCommands() {
    auto cmds = UserDataManager::instance().getCommands();
    vector<SvcDTO::SavedCommandDTO> result;
    result.reserve(cmds.size());
    for (const auto& c : cmds) {
        string tool_name = resolveToolName(c.tool_id);
        result.push_back(toDTO(c, tool_name));
    }
    // Sort by note (description) for consistent ordering
    sort(result.begin(), result.end(),
         [](const SvcDTO::SavedCommandDTO& a,
            const SvcDTO::SavedCommandDTO& b) {
             return a.note < b.note;
         });
    return result;
}

SvcDTO::SavedCommandDTO getCommandById(int id) {
    auto cmd = UserDataManager::instance().getCommandById(id);
    if (cmd.id == 0) return {};  // not found (nextCommandId starts at 1)
    string tool_name = resolveToolName(cmd.tool_id);
    return toDTO(cmd, tool_name);
}

int saveCommand(int tool_id, const string& command, const string& note) {
    return UserDataManager::instance().saveCommand(tool_id, command, note);
}

bool updateCommand(int id, int tool_id, const string& command,
                   const string& note) {
    return UserDataManager::instance().updateCommand(id, tool_id,
                                                      command, note);
}

bool deleteCommand(int id) {
    return UserDataManager::instance().deleteCommand(id);
}

} // namespace SvcSavedCommands
