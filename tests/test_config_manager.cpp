#include "doctest.h"
#include "config_manager.h"
#include "fixtures.h"
#include <fstream>
#include <string>

using json = nlohmann::json;

TEST_CASE("ConfigManager — load non-existent file creates defaults") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();

    ConfigManager cm(configPath);
    CHECK(cm.get<int>("colors", -1) == 1);
    CHECK(cm.get<std::string>("App.info.version", "") == "V1.0.1");
    CHECK(cm.get<int>("disclaimer_accepted", -1) == 0);
}

TEST_CASE("ConfigManager — set and get round-trip") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();

    ConfigManager cm(configPath);
    cm.set("test_key", 42);
    cm.set("nested.key", std::string("hello"));
    cm.set("flag", true);

    CHECK(cm.get<int>("test_key", 0) == 42);
    CHECK(cm.get<std::string>("nested.key", "") == "hello");
    CHECK(cm.get<bool>("flag", false) == true);
}

TEST_CASE("ConfigManager — save then load preserves values") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();

    {
        ConfigManager cm(configPath);
        cm.set("custom.value", std::string("proper"));
        cm.save();
    }

    ConfigManager cm(configPath);
    CHECK(cm.get<std::string>("custom.value", "") == "proper");
}

TEST_CASE("ConfigManager — get with missing key returns default") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();

    ConfigManager cm(configPath);
    CHECK(cm.get<int>("nonexistent.key", -1) == -1);
    CHECK(cm.get<std::string>("missing", "fallback") == "fallback");
}

TEST_CASE("ConfigManager — save returns true on success") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();

    ConfigManager cm(configPath);
    bool saved = cm.save();
    CHECK(saved == true);

    std::ifstream in(configPath);
    REQUIRE(in.is_open());
    json j;
    in >> j;
    CHECK(j.contains("colors"));
}

TEST_CASE("ConfigManager — load with corrupt JSON recovers with defaults") {
    test_fixtures::TempDirectory dir;
    auto configPath = test_fixtures::createTempFile(dir, "config.json",
        "{ invalid json }");

    ConfigManager cm(configPath);
    CHECK(cm.get<int>("colors", -1) == 1);
}
