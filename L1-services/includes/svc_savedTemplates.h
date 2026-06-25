/*
 *  tguide — svc_savedTemplates.h
 *  written by voidoxin
 *
 *  Service layer for saved templates.
 *  Connects UI_tools → L0-core saved template storage.
 */

#pragma once
#include <string>
#include <vector>
#include "svc_dto.h"

namespace SvcSavedTemplates {

    // Retrieve all saved templates, with tool names resolved
    std::vector<SvcDTO::SavedTemplateDTO> getAllTemplates();

    // Retrieve saved templates for a specific tool
    std::vector<SvcDTO::SavedTemplateDTO> getTemplatesByToolId(int toolId);

    // Save a new template. Returns new id on success, -1 on failure.
    int  saveTemplate(int tool_id, const std::string& name,
                      const std::string& content, const std::string& description);

    // Delete a template by id. Returns true on success.
    bool deleteTemplate(int id);

} // namespace SvcSavedTemplates
