/*
 *  tguide — svc_tools.cpp
 *  service bridge: UI_tools → ToolD queries
 *
 *  written by voidoxin
 */

#include "../includes/svc_tools.h"
#include "../../L0-core/include/DatabaseManager.h"
#include "../../L0-core/include/path_resolver.h"
#include <algorithm>
#include <cctype>
#include <set>

using namespace std;

// ── L0→DTO mappers ────────────────────────────────────────────────────
// These convert L0-core types to L1 DTOs, keeping L0 types internal to L1.

static SvcDTO::ToolDTO toDTO(const Tool& t) {
    return { t.id, t.name, t.category, t.short_desc, t.description, t.flags_all };
}

static std::vector<SvcDTO::ToolDTO> toDTOs(const std::vector<Tool>& tools) {
    std::vector<SvcDTO::ToolDTO> result;
    result.reserve(tools.size());
    for (const auto& t : tools) result.push_back(toDTO(t));
    return result;
}

// ── search index — held in memory for the session ─────────────────────────
// searchTools() stub does not populate this yet; clearSearchIndex() exists
// so callers can reset state before a future reloadDatabase() call.
struct SearchIndex {
    vector<string> index;
    vector<Tool> tools;
    bool ready = false;
};

static SearchIndex& getIndex() {
    static SearchIndex idx;
    return idx;
}

void clearSearchIndex() {
    getIndex().index.clear();
    getIndex().tools.clear();
    getIndex().ready = false;
}

namespace SvcTools {                              
// ── CATEGORIES ──────────────────────────────────────────────────────────────
                                                   vector<string> getCategories() {
    ToolD db(PathResolver::dbFile().string());
    ToolResults res = db.getAll();

    set<string>    seen;   // lowercased keys for case-insensitive dedup
    vector<string> cats;

    for (const Tool& t : res.items) {
        if (t.category.empty()) continue;
        string key = t.category;
        transform(key.begin(), key.end(), key.begin(), ::tolower);
        if (seen.insert(key).second)
            cats.push_back(t.category);  // preserve original casing
    }

    sort(cats.begin(), cats.end(), [](const string& a, const string& b) {
        string la = a, lb = b;
        transform(la.begin(), la.end(), la.begin(), ::tolower);
        transform(lb.begin(), lb.end(), lb.begin(), ::tolower);
        return la < lb;
    });

    return cats;
}

// ── TOOLS BY CATEGORY ───────────────────────────────────────────────────────

vector<SvcDTO::ToolDTO> getToolsByCategory(const string& category) {
    ToolD db(PathResolver::dbFile().string());
    ToolResults res = db.getWhere({"category"}, {category});
    return toDTOs(res.items);
}

// ── SEARCH ──────────────────────────────────────────────────────────────────

// TODO: implement search algorithm
vector<SvcDTO::ToolDTO> searchTools(const string& query) {
    (void)query; return {};
}

} // namespace SvcTools