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
#include <string>
#include <vector>

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

namespace SvcTools {
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

// ── CATEGORY LIST (from categories table) ─────────────────────────────────

vector<SvcDTO::CategoryDTO> getCategoryList() {
    CategoryD db(PathResolver::dbFile().string());
    CategoryResults res = db.getAll();

    // Sort by display_order, then alphabetically by name
    sort(res.items.begin(), res.items.end(),
         [](const Category& a, const Category& b) {
             if (a.display_order != b.display_order)
                 return a.display_order < b.display_order;
             return a.name < b.name;
         });

    vector<SvcDTO::CategoryDTO> result;
    result.reserve(res.items.size());
    for (const auto& c : res.items) {
        result.push_back({c.id, c.name, c.description});
    }
    return result;
}

} // namespace SvcTools
