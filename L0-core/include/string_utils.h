#pragma once
#include <string>

enum class StringID : int {
    // Tools entry screen
    TOOLS_BROWSE_CATEGORY,
    TOOLS_SEARCH,
    TOOLS_VIEW_ALL,
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

    // Saved Templates screen
    SAVED_TEMPLATES_LIST,
    SAVED_TEMPLATES_ADD,
    SAVED_TEMPLATES_EMPTY,
    SAVED_TEMPLATES_DELETED,
    SAVED_TEMPLATES_CONFIRM_DELETE,
    SAVED_TEMPLATES_NAME_PROMPT,
    SAVED_TEMPLATES_DESC_PROMPT,
    SAVED_TEMPLATES_CONTENT_PROMPT,
    SAVED_TEMPLATES_BROWSE,
    SAVED_TEMPLATES_SEARCH_TOOL,
    SAVED_TEMPLATES_MAKE_OWN,

    _COUNT  // must be last
};

namespace Strings {
    const std::string& get(StringID id);
}
