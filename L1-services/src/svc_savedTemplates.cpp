/*
 *  tguide — svc_savedTemplates.cpp
 *  written by voidoxin
 *
 *  Service layer for the saved templates.
 *  Connects UI_tools → L0-core user template storage.
 */

#include <algorithm>
#include <string>
#include <vector>

#include "../../L0-core/include/DatabaseManager.h"
#include "../../L0-core/include/path_resolver.h"
#include "../../L0-core/include/UserDataManager.h"
#include "../includes/svc_savedTemplates.h"

using namespace std;

// ── L0→DTO mapper ────────────────────────────────────────────────────

static SvcDTO::SavedTemplateDTO toDTO(const SavedTemplate& t,
                                       const string& tool_name) {
    return { t.id, t.tool_id, t.name, t.content, t.description, tool_name };
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

namespace SvcSavedTemplates {

vector<SvcDTO::SavedTemplateDTO> getAllTemplates() {
    auto tmpls = UserDataManager::instance().getTemplates();
    vector<SvcDTO::SavedTemplateDTO> result;
    result.reserve(tmpls.size());
    for (const auto& t : tmpls) {
        string tool_name = resolveToolName(t.tool_id);
        result.push_back(toDTO(t, tool_name));
    }
    sort(result.begin(), result.end(),
         [](const SvcDTO::SavedTemplateDTO& a,
            const SvcDTO::SavedTemplateDTO& b) {
             return a.name < b.name;
         });
    return result;
}

vector<SvcDTO::SavedTemplateDTO> getTemplatesByToolId(int toolId) {
    auto tmpls = UserDataManager::instance().getTemplatesByToolId(toolId);
    vector<SvcDTO::SavedTemplateDTO> result;
    result.reserve(tmpls.size());
    string tool_name = resolveToolName(toolId);
    for (const auto& t : tmpls) {
        result.push_back(toDTO(t, tool_name));
    }
    return result;
}

int saveTemplate(int tool_id, const string& name,
                 const string& content, const string& description) {
    return UserDataManager::instance().saveTemplate(tool_id, name, content, description);
}

bool deleteTemplate(int id) {
    return UserDataManager::instance().deleteTemplate(id);
}

} // namespace SvcSavedTemplates
