/*
 *  tguide — test_saved_scripts.cpp
 *  Tests for SvcSavedScripts service layer
 */

#include "doctest.h"
#include "UserDataManager.h"
#include "ErrorHandler.h"
#include "fixtures.h"
#include "../../L1-services/includes/svc_savedScripts.h"
#include <filesystem>

// Helper to reinitialize singleton with temp paths
static void initTempPaths(const std::string& cmds, const std::string& scripts) {
    UserDataManager::instance().init(cmds, scripts);
    UserDataManager::instance().load();
}

TEST_CASE("SvcSavedScripts — save and list all scripts") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    int id1 = SvcSavedScripts::saveScript("scan", "/tmp/scan.sh", "Network scan");
    CHECK(id1 > 0);
    int id2 = SvcSavedScripts::saveScript("brute", "/tmp/brute.sh", "Brute force");
    CHECK(id2 > 0);
    CHECK(id2 != id1);

    auto all = SvcSavedScripts::getAllScripts();
    CHECK(all.size() == 2);
    CHECK(!all[0].name.empty());
    CHECK(!all[0].path.empty());
    CHECK(!all[0].note.empty());
}

TEST_CASE("SvcSavedScripts — get script by id") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    int id = SvcSavedScripts::saveScript("test", "/tmp/test.sh", "Test note");
    REQUIRE(id > 0);

    auto s = SvcSavedScripts::getScriptById(id);
    CHECK(s.id == id);
    CHECK(s.name == "test");
    CHECK(s.path == "/tmp/test.sh");
    CHECK(s.note == "Test note");
}

TEST_CASE("SvcSavedScripts — get non-existent id returns empty") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    auto s = SvcSavedScripts::getScriptById(999);
    CHECK(s.id == 0);
    CHECK(s.name.empty());
    CHECK(s.path.empty());
    CHECK(s.note.empty());
}

TEST_CASE("SvcSavedScripts — update script") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    int id = SvcSavedScripts::saveScript("old", "/tmp/old.sh", "old note");
    REQUIRE(id > 0);

    bool ok = SvcSavedScripts::updateScript(id, "new", "/tmp/new.sh", "new note");
    CHECK(ok == true);

    auto s = SvcSavedScripts::getScriptById(id);
    CHECK(s.name == "new");
    CHECK(s.path == "/tmp/new.sh");
    CHECK(s.note == "new note");
}

TEST_CASE("SvcSavedScripts — update non-existent id returns false") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    bool ok = SvcSavedScripts::updateScript(999, "x", "x", "x");
    CHECK(ok == false);
}

TEST_CASE("SvcSavedScripts — delete script") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    int id = SvcSavedScripts::saveScript("del", "/tmp/del.sh", "to delete");
    REQUIRE(id > 0);
    CHECK(SvcSavedScripts::getAllScripts().size() == 1);

    bool ok = SvcSavedScripts::deleteScript(id);
    CHECK(ok == true);
    CHECK(SvcSavedScripts::getAllScripts().empty());
}

TEST_CASE("SvcSavedScripts — delete non-existent id returns false") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    bool ok = SvcSavedScripts::deleteScript(999);
    CHECK(ok == false);
}

TEST_CASE("SvcSavedScripts — scripts sorted by note") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    SvcSavedScripts::saveScript("z", "/tmp/z.sh", "Zebra note");
    SvcSavedScripts::saveScript("a", "/tmp/a.sh", "Alpha note");
    SvcSavedScripts::saveScript("m", "/tmp/m.sh", "Middle note");

    auto all = SvcSavedScripts::getAllScripts();
    REQUIRE(all.size() >= 2);
    for (size_t i = 1; i < all.size(); ++i)
        CHECK(all[i - 1].note <= all[i].note);
}

TEST_CASE("SvcSavedScripts — empty script list") {
    test_fixtures::TempDirectory dir;
    test_fixtures::ErrorHandlerSpy spy;

    auto cmdsPath = (dir.path / "commands.json").string();
    auto scrPath  = (dir.path / "scripts.json").string();
    initTempPaths(cmdsPath, scrPath);

    auto all = SvcSavedScripts::getAllScripts();
    CHECK(all.empty());
}
