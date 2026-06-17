#include "doctest.h"
#include "path_resolver.h"
#include <filesystem>

namespace fs = std::filesystem;

TEST_CASE("installDbFile returns non-empty path") {
    fs::path p = PathResolver::installDbFile();
    CHECK(p.empty() == false);
    CHECK(p.string() != ".");
}

TEST_CASE("installDbFile returns absolute path") {
    fs::path p = PathResolver::installDbFile();
    CHECK(p.is_absolute() == true);
}

TEST_CASE("installDbFile ends with tguide.db") {
    fs::path p = PathResolver::installDbFile();
    CHECK(p.filename() == "tguide.db");
}

TEST_CASE("installDbFile path contains tguide directory") {
    fs::path p = PathResolver::installDbFile();
    CHECK(p.parent_path().filename() == "tguide");
}
