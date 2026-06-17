/*
 *  tguide — svc_savedScripts.h
 *  written by voidoxin
 *
 *  Service layer for the Saved Scripts screen.
 *  Handles all business logic between UI_savedScripts and L0-core:
 *    - load user-saved scripts from storage
 *    - add, edit, delete, and export saved scripts
 *    - persist changes back to user storage
 */

#pragma once
#include <string>
#include <vector>
#include "svc_dto.h"

namespace SvcSavedScripts {

    // Retrieve all saved scripts, sorted by note
    std::vector<SvcDTO::SavedScriptDTO> getAllScripts();

    // Retrieve a single saved script by id
    SvcDTO::SavedScriptDTO getScriptById(int id);

    // Save a new script. Returns new id on success, -1 on failure.
    int  saveScript(const std::string& name, const std::string& path,
                    const std::string& note);

    // Update an existing script. Returns true on success.
    bool updateScript(int id, const std::string& name, const std::string& path,
                      const std::string& note);

    // Delete a script by id. Returns true on success.
    bool deleteScript(int id);

} // namespace SvcSavedScripts