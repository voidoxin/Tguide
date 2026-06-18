#pragma once
#include <string>

enum class StringID : int {
    // Tools entry screen
    TOOLS_BROWSE_CATEGORY,
    TOOLS_SEARCH,
    TOOLS_BACK,
    TOOLS_INVALID_CHOICE,
    TOOLS_AMBIGUOUS,
    TOOLS_PROMPT,

    // Saved Commands screen
    SAVED_COMMANDS_LIST,      // "List Saved Commands"
    SAVED_COMMANDS_ADD,       // "Add New Command"
    SAVED_COMMANDS_DELETE,    // "Delete Command"
    SAVED_COMMANDS_EMPTY,     // "no saved commands."
    SAVED_COMMANDS_DELETED,   // "command deleted."
    SAVED_CONFIRM_DELETE,     // "delete this command? (y/n)"
    SAVED_COMMAND_PROMPT,     // "command"
    SAVED_NOTE_PROMPT,        // "note"
    SAVED_TOOL_PROMPT,        // "tool name"
    SAVED_PREVIEW,            // "command preview"
    SAVED_EDIT_NOTE,          // "edit note"

    // Saved Scripts screen
    SAVED_SCRIPTS_LIST,       // "List Saved Scripts"
    SAVED_SCRIPTS_EMPTY,      // "no saved scripts."
    SAVED_SCRIPT_NAME_PROMPT, // "script name"
    SAVED_SCRIPT_PATH_PROMPT, // "path"

    _COUNT  // must be last
};

namespace Strings {
    const std::string& get(StringID id);
}
