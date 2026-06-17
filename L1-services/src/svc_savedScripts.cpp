/*
 *  tguide — svc_savedScripts.cpp
 *  written by voidoxin
 *
 *  Service layer for the Saved Scripts screen.
 *  Connects UI_savedScripts → L0-core user script storage.
 */

#include <algorithm>
#include <string>
#include <vector>

#include "../../L0-core/include/UserDataManager.h"
#include "../includes/svc_savedScripts.h"

using namespace std;

// ── L0→DTO mapper ────────────────────────────────────────────────────

static SvcDTO::SavedScriptDTO toDTO(const SavedScript& s) {
    return { s.id, s.name, s.path, s.note };
}

// ==================== NAMESPACE ====================

namespace SvcSavedScripts {

vector<SvcDTO::SavedScriptDTO> getAllScripts() {
    auto scripts = UserDataManager::instance().getScripts();
    vector<SvcDTO::SavedScriptDTO> result;
    result.reserve(scripts.size());
    for (const auto& s : scripts)
        result.push_back(toDTO(s));
    // Sort by note (description) for consistent ordering
    sort(result.begin(), result.end(),
         [](const SvcDTO::SavedScriptDTO& a,
            const SvcDTO::SavedScriptDTO& b) {
             return a.note < b.note;
         });
    return result;
}

SvcDTO::SavedScriptDTO getScriptById(int id) {
    auto s = UserDataManager::instance().getScriptById(id);
    if (s.id == 0) return {};  // not found (nextScriptId starts at 1)
    return toDTO(s);
}

int saveScript(const string& name, const string& path, const string& note) {
    return UserDataManager::instance().saveScript(name, path, note);
}

bool updateScript(int id, const string& name, const string& path,
                  const string& note) {
    return UserDataManager::instance().updateScript(id, name, path, note);
}

bool deleteScript(int id) {
    return UserDataManager::instance().deleteScript(id);
}

} // namespace SvcSavedScripts