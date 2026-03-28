/*
 *  tguide — svc_tools.cpp
 *  service bridge: UI_tools → ToolD queries
 *
 *  written by voidoxin
 */

#include "../includes/svc_tools.h"
#include <algorithm>
#include <set>
#include <cctype>

using namespace std;

namespace SvcTools {                              
// ── CATEGORIES ──────────────────────────────────────────────────────────────
                                                  vector<string> getCategories(ToolD& db) {
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

vector<Tool> getToolsByCategory(ToolD& db, const string& category) {
    ToolResults res = db.getWhere({"category"}, {category});
    return res.items;
}

// ── SEARCH ──────────────────────────────────────────────────────────────────

// TODO: implement search algorithm
vector<Tool> searchTools(ToolD& db, const string& query) {
    (void)db; (void)query; return {};
}

} // namespace SvcTools