/*
 *  tguide — svc_tools.h
 *  service layer for the Tools screen — bridges UI_tools to L0-core
 *
 *  written by voidoxin
 */

#pragma once

#include "svc_dto.h"
#include <string>
#include <vector>

namespace SvcTools {

    // returns sorted distinct category strings from all tools, skipping empty
    std::vector<std::string> getCategories();

    // returns all tools whose category matches the given string (case-sensitive DB match)
    std::vector<SvcDTO::ToolDTO> getToolsByCategory(const std::string& category);

    // TODO: implement search algorithm
    // returns empty vector until algorithm is ready
    std::vector<SvcDTO::ToolDTO> searchTools(const std::string& query);
} // namespace SvcTools