/*
 *  tguide — svc_tools.h
 *  service layer for the Tools screen — bridges UI_tools to L0-core
 *
 *  written by voidoxin
 */

#pragma once

#include "../../L0-core/include/DatabaseManager.h"
#include <string>
#include <vector>

// clear the in-memory search index — safe to call before rebuild or at shutdown
void clearSearchIndex();

namespace SvcTools {

    // returns sorted distinct category strings from all tools, skipping empty
    std::vector<std::string> getCategories();

    // returns all tools whose category matches the given string (case-sensitive DB match)
    std::vector<Tool> getToolsByCategory(const std::string& category);

    // TODO: implement search algorithm
    // returns empty vector until algorithm is ready
    std::vector<Tool> searchTools(const std::string& query);
} // namespace SvcTools