#include "doctest.h"
#include "string_utils.h"
#include <string>

TEST_CASE("Strings — every StringID returns non-empty string") {
    for (int i = 0; i < static_cast<int>(StringID::_COUNT); ++i) {
        CHECK(!Strings::get(static_cast<StringID>(i)).empty());
    }
}

TEST_CASE("Strings — returned values match expected text") {
    CHECK(Strings::get(StringID::TOOLS_BROWSE_CATEGORY) == "Browse by Category");
    CHECK(Strings::get(StringID::TOOLS_SEARCH) == "Search");
    CHECK(Strings::get(StringID::TOOLS_BACK) == "Back");
    CHECK(Strings::get(StringID::TOOLS_PROMPT) == "\u2192");
}

TEST_CASE("Strings — reference stability across calls") {
    const std::string& first = Strings::get(StringID::TOOLS_BROWSE_CATEGORY);
    const std::string& second = Strings::get(StringID::TOOLS_BROWSE_CATEGORY);
    CHECK(&first == &second);
    CHECK(first == second);
}
