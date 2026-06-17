/*
 *  tguide — svc_savedCommands.h
 *  written by voidoxin
 *
 *  Service layer for the Saved Commands screen.
 *  Handles all business logic between UI_savedCommands and L0-core:
 *    - load user-saved commands from storage
 *    - add, edit, delete, and search saved commands
 *    - persist changes back to user storage
 */

#pragma once
#include <string>
#include <vector>
#include "svc_dto.h"

namespace SvcSavedCommands {

    // Retrieve all saved commands, with tool names resolved
    std::vector<SvcDTO::SavedCommandDTO> getAllCommands();

    // Retrieve a single saved command by id
    SvcDTO::SavedCommandDTO getCommandById(int id);

    // Save a new command. Returns new id on success, -1 on failure.
    int  saveCommand(int tool_id, const std::string& command,
                     const std::string& note);

    // Update an existing command. Returns true on success.
    bool updateCommand(int id, int tool_id, const std::string& command,
                       const std::string& note);

    // Delete a command by id. Returns true on success.
    bool deleteCommand(int id);

} // namespace SvcSavedCommands
