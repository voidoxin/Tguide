#pragma once
#include <string>

enum class StringID : int {
    // Tools entry screen
    TOOLS_BROWSE_CATEGORY,
    TOOLS_SEARCH,
    TOOLS_FILTER,
    TOOLS_BACK,
    TOOLS_COMING_SOON,
    TOOLS_INVALID_CHOICE,
    TOOLS_AMBIGUOUS,
    TOOLS_PROMPT,

    _COUNT  // must be last
};

namespace Strings {
    const std::string& get(StringID id);
}
