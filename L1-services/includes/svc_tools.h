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

    // ── TEMPLATE FILL ───────────────────────────────────────────────────
    // Build a command string from a tool, template, and user-provided values
    std::string buildCommand(const SvcDTO::ToolDTO& tool,
                             const SvcDTO::TemplateDTO& templ,
                             const std::string& target,
                             const std::string& port);

    // Save a built command to UserDataManager saved commands; returns the new id or -1
    int saveTemplateCommand(int toolId, const std::string& command, const std::string& note);

    // ── VULNERABILITIES ────────────────────────────────────────────────
    // Returns all vulnerabilities, sorted by name
    std::vector<SvcDTO::VulnerabilityDTO> getAllVulnerabilities();

    // Search vulnerabilities by name or metasploit path (exact match)
    std::vector<SvcDTO::VulnerabilityDTO> searchVulnerabilities(const std::string& query);

    // Filter vulnerabilities by a column/value pair (severity, access, or platform)
    std::vector<SvcDTO::VulnerabilityDTO> filterVulnerabilities(const std::string& column,
                                                                 const std::string& value);

    // Get sorted distinct string values for a given column (severity, access, platform)
    std::vector<std::string> getDistinctValues(const std::string& column);
} // namespace SvcTools
