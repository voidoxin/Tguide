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

    // returns all categories with id, name, and description, sorted by
    // display_order then name — uses the categories table (STEP-07)
    std::vector<SvcDTO::CategoryDTO> getCategoryList();

    // returns all tools whose category matches the given string (case-sensitive DB match)
    std::vector<SvcDTO::ToolDTO> getToolsByCategory(const std::string& category);

    // TODO: implement search algorithm
    // returns empty vector until algorithm is ready
    std::vector<SvcDTO::ToolDTO> searchTools(const std::string& query);

    // ── TOOL DETAIL ─────────────────────────────────────────────────────
    // Fetch a single tool by its primary key
    SvcDTO::ToolDTO getToolById(int id);

    // Fetch all flags belonging to a tool, ordered by id
    std::vector<SvcDTO::ToolFlagDTO> getFlagsByToolId(int toolId);

    // Fetch all templates belonging to a tool, ordered by id
    std::vector<SvcDTO::TemplateDTO> getTemplatesByToolId(int toolId);
} // namespace SvcTools
