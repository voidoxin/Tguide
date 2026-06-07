#include "doctest.h"
#include "UserDataManager.h"
#include "ErrorHandler.h"
#include "fixtures.h"
#include <filesystem>

TEST_CASE("UserDataManager — load non-existent files creates empty data") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager udm(cmdsPath, scrPath);
    bool loaded = udm.load();

    CHECK(loaded == true);
    CHECK(udm.getCommands().empty());
    CHECK(udm.getScripts().empty());
}

TEST_CASE("UserDataManager — save and load commands round-trip") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager udm(cmdsPath, scrPath);
    REQUIRE(udm.load() == true);

    int id = udm.saveCommand(1, "nmap -sV target", "Quick scan");
    CHECK(id > 0);

    UserDataManager udm2(cmdsPath, scrPath);
    REQUIRE(udm2.load() == true);

    auto cmds = udm2.getCommands();
    CHECK(cmds.size() == 1);
    CHECK(cmds[0].id == id);
    CHECK(cmds[0].tool_id == 1);
    CHECK(cmds[0].command == "nmap -sV target");
    CHECK(cmds[0].note == "Quick scan");
}

TEST_CASE("UserDataManager — delete command persists") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager udm(cmdsPath, scrPath);
    REQUIRE(udm.load() == true);

    int id = udm.saveCommand(1, "test", "note");
    CHECK(id > 0);
    CHECK(udm.getCommands().size() == 1);

    bool deleted = udm.deleteCommand(id);
    CHECK(deleted == true);
    CHECK(udm.getCommands().empty());
}

TEST_CASE("UserDataManager — save and load scripts round-trip") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager udm(cmdsPath, scrPath);
    REQUIRE(udm.load() == true);

    int id = udm.saveScript("myscript", "/tmp/myscript.sh", "Auto-generated");
    CHECK(id > 0);

    UserDataManager udm2(cmdsPath, scrPath);
    REQUIRE(udm2.load() == true);

    auto scripts = udm2.getScripts();
    CHECK(scripts.size() == 1);
    CHECK(scripts[0].name == "myscript");
    CHECK(scripts[0].path == "/tmp/myscript.sh");
}

TEST_CASE("UserDataManager — delete non-existent id returns false") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager udm(cmdsPath, scrPath);
    REQUIRE(udm.load() == true);

    bool deleted = udm.deleteCommand(999);
    CHECK(deleted == false);

    bool deletedScript = udm.deleteScript(999);
    CHECK(deletedScript == false);
}

TEST_CASE("UserDataManager — empty command path returns -1") {
    test_fixtures::ErrorHandlerSpy spy;

    UserDataManager udm("", "");
    // load() returns false because empty paths can't be written to — this is expected
    udm.load();

    int id = udm.saveCommand(1, "cmd", "note");
    CHECK(id == -1);
}
