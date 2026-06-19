#pragma once
#include <string>
#include <vector>
#include "L1-services/includes/svc_dto.h"

// Export saved scripts to a file in the given format. Returns true on success.
bool exportScriptsToText(const std::vector<SvcDTO::SavedScriptDTO>& scripts, const std::string& filePath);
bool exportScriptsToJson(const std::vector<SvcDTO::SavedScriptDTO>& scripts, const std::string& filePath);
bool exportScriptsToYaml(const std::vector<SvcDTO::SavedScriptDTO>& scripts, const std::string& filePath);
bool exportScriptsToCsv(const std::vector<SvcDTO::SavedScriptDTO>& scripts, const std::string& filePath);

// Export raw display text content to a file. Returns true on success.
bool exportTextContent(const std::string& content, const std::string& filePath);
