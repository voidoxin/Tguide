/*
 *  tguide — test_saved_commands.cpp
 *  Tests for SvcSavedCommands service layer
 */

#include "doctest.h"
#include "UserDataManager.h"
#include "ErrorHandler.h"
#include "fixtures.h"
#include "../../L1-services/includes/svc_savedCommands.h"
#include <filesystem>

TEST_CASE("SvcSavedCommands — save and list all commands") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    // Reinitialize the singleton for this test
    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    // Save a command through the service layer
    int id = SvcSavedCommands::saveCommand(1, "nmap -sV 10.0.0.1", "Quick scan");
    CHECK(id > 0);

    // Save another
    int id2 = SvcSavedCommands::saveCommand(2, "hydra -l admin -P wordlist.txt 10.0.0.1",
                                             "Hydra brute force");
    CHECK(id2 > 0);
    CHECK(id2 != id);  // distinct ids

    // Retrieve all
    auto all = SvcSavedCommands::getAllCommands();
    CHECK(all.size() == 2);

    // Verify DTO fields (tool_name may be empty if test DB is unavailable)
    CHECK(all[0].id > 0);
    CHECK(all[0].tool_id > 0);
    CHECK(!all[0].command.empty());
    CHECK(!all[0].note.empty());

    // Verify both commands are present
    bool found_first  = false;
    bool found_second = false;
    for (const auto& cmd : all) {
        if (cmd.command.find("nmap") != std::string::npos) found_first  = true;
        if (cmd.command.find("hydra") != std::string::npos) found_second = true;
    }
    CHECK(found_first);
    CHECK(found_second);
}

TEST_CASE("SvcSavedCommands — get command by id") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    int id = SvcSavedCommands::saveCommand(1, "sqlmap -u http://target.com --dbs",
                                            "SQL injection scan");
    REQUIRE(id > 0);

    auto cmd = SvcSavedCommands::getCommandById(id);
    CHECK(cmd.id == id);
    CHECK(cmd.tool_id == 1);
    CHECK(cmd.command == "sqlmap -u http://target.com --dbs");
    CHECK(cmd.note == "SQL injection scan");
}

TEST_CASE("SvcSavedCommands — get non-existent id returns empty") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    auto cmd = SvcSavedCommands::getCommandById(999);
    CHECK(cmd.id == 0);
    CHECK(cmd.command.empty());
    CHECK(cmd.tool_name.empty());
    CHECK(cmd.note.empty());
    CHECK(cmd.tool_id == 0);
}

TEST_CASE("SvcSavedCommands — update command") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    int id = SvcSavedCommands::saveCommand(1, "old command", "old note");
    REQUIRE(id > 0);

    bool updated = SvcSavedCommands::updateCommand(id, 2, "new command", "new note");
    CHECK(updated == true);

    auto cmd = SvcSavedCommands::getCommandById(id);
    CHECK(cmd.tool_id == 2);
    CHECK(cmd.command == "new command");
    CHECK(cmd.note == "new note");
}

TEST_CASE("SvcSavedCommands — update non-existent id returns false") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    bool updated = SvcSavedCommands::updateCommand(999, 1, "cmd", "note");
    CHECK(updated == false);
}

TEST_CASE("SvcSavedCommands — delete command") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    int id = SvcSavedCommands::saveCommand(1, "delete me", "to be deleted");
    REQUIRE(id > 0);
    CHECK(SvcSavedCommands::getAllCommands().size() == 1);

    bool deleted = SvcSavedCommands::deleteCommand(id);
    CHECK(deleted == true);
    CHECK(SvcSavedCommands::getAllCommands().empty());
}

TEST_CASE("SvcSavedCommands — delete non-existent id returns false") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    bool deleted = SvcSavedCommands::deleteCommand(999);
    CHECK(deleted == false);
}

TEST_CASE("SvcSavedCommands — commands sorted by note") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    // Insert in non-alphabetical note order
    SvcSavedCommands::saveCommand(1, "cmd z", "Zebra note");
    SvcSavedCommands::saveCommand(1, "cmd a", "Alpha note");
    SvcSavedCommands::saveCommand(1, "cmd m", "Middle note");

    auto all = SvcSavedCommands::getAllCommands();
    REQUIRE(all.size() >= 2);
    for (size_t i = 1; i < all.size(); ++i) {
        CHECK(all[i - 1].note <= all[i].note);
    }
}

TEST_CASE("SvcSavedCommands — empty command list") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();

    UserDataManager::instance().init(cmdsPath, scrPath);
    REQUIRE(UserDataManager::instance().load() == true);

    auto all = SvcSavedCommands::getAllCommands();
    CHECK(all.empty());
}
