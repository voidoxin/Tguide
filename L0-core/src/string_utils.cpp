#include "string_utils.h"
#include <array>
#include <cassert>

static const std::array<std::string, static_cast<size_t>(StringID::_COUNT)> s_strings = {{
    /* TOOLS_BROWSE_CATEGORY */ "Browse by Category",
    /* TOOLS_SEARCH          */ "Search",
    /* TOOLS_FILTER          */ "Filter",
    /* TOOLS_BACK            */ "Back",
    /* TOOLS_COMING_SOON     */ "coming soon \u2014 not yet implemented",
    /* TOOLS_INVALID_CHOICE  */ "invalid choice \u2014 try again.",
    /* TOOLS_AMBIGUOUS       */ "ambiguous \u2014 be more specific.",
    /* TOOLS_PROMPT          */ "\u2192",
}};

static_assert(s_strings.size() == static_cast<size_t>(StringID::_COUNT), "string table size must match StringID enum count");

namespace Strings {
    const std::string& get(StringID id) {
        auto idx = static_cast<size_t>(id);
        assert(idx < s_strings.size() && "StringID out of range");
        return s_strings[idx];
    }
}
