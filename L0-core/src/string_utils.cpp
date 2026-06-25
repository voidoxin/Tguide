#include "string_utils.h"
#include <array>
#include <cassert>

static const std::array<std::string, static_cast<size_t>(StringID::_COUNT)> s_strings = {{
    /* TOOLS_BROWSE_CATEGORY */ "Browse by Category",
    /* TOOLS_SEARCH          */ "Search",
    /* TOOLS_VIEW_ALL        */ "View All Tools",
    /* TOOLS_BACK            */ "Back",
    /* TOOLS_INVALID_CHOICE  */ "invalid choice \u2014 try again.",
    /* TOOLS_AMBIGUOUS       */ "ambiguous \u2014 be more specific.",
    /* TOOLS_PROMPT          */ "\u2192",

    /* SAVED_COMMANDS_LIST   */ "List Saved Commands",
    /* SAVED_COMMANDS_ADD    */ "Add New Command",
    /* SAVED_COMMANDS_DELETE */ "Delete Command",
    /* SAVED_COMMANDS_EMPTY  */ "no saved commands.",
    /* SAVED_COMMANDS_DELETED*/ "command deleted.",
    /* SAVED_CONFIRM_DELETE  */ "delete this command? (y/n)",
    /* SAVED_COMMAND_PROMPT  */ "command",
    /* SAVED_NOTE_PROMPT     */ "note",
    /* SAVED_TOOL_PROMPT     */ "tool name",
    /* SAVED_PREVIEW         */ "command preview",
    /* SAVED_EDIT_NOTE       */ "edit note",

    /* SAVED_SCRIPTS_LIST    */ "List Saved Scripts",
    /* SAVED_SCRIPTS_EMPTY   */ "no saved scripts.",
    /* SAVED_SCRIPT_NAME_PROMPT*/ "script name",
    /* SAVED_SCRIPT_PATH_PROMPT*/ "path",

    /* SAVED_TEMPLATES_LIST          */ "My Templates",
    /* SAVED_TEMPLATES_ADD           */ "Add New Template",
    /* SAVED_TEMPLATES_EMPTY         */ "no saved templates.",
    /* SAVED_TEMPLATES_DELETED       */ "template deleted.",
    /* SAVED_TEMPLATES_CONFIRM_DELETE*/ "delete this template? (y/n)",
    /* SAVED_TEMPLATES_NAME_PROMPT   */ "template name",
    /* SAVED_TEMPLATES_DESC_PROMPT   */ "description",
    /* SAVED_TEMPLATES_CONTENT_PROMPT*/ "template content",
    /* SAVED_TEMPLATES_BROWSE        */ "Browse by Category",
    /* SAVED_TEMPLATES_SEARCH_TOOL   */ "Search Tool",
    /* SAVED_TEMPLATES_MAKE_OWN      */ "Make Own Template",
}};

static_assert(s_strings.size() == static_cast<size_t>(StringID::_COUNT), "string table size must match StringID enum count");

namespace Strings {
    const std::string& get(StringID id) {
        auto idx = static_cast<size_t>(id);
        assert(idx < s_strings.size() && "StringID out of range");
        return s_strings[idx];
    }
}
