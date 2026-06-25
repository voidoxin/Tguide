/*
 *  tguide — test_ui_quit.cpp
 *  Tests for quit / back / menu exit behaviour in UI screens
 *
 *  written by voidoxin
 */

#include "doctest.h"
#include "UI_input.h"
#include "UI_disclaimer.h"
#include "UI_errorHandling.h"
#include "config_manager.h"
#include "fixtures.h"
#include <sstream>
#include <iostream>

using namespace std;

// =============================================================
// handleQuit  —  standalone goodbye screen
// =============================================================

TEST_CASE("handleQuit — prints goodbye") {
    stringstream buffer;
    streambuf* old = cout.rdbuf(buffer.rdbuf());

    handleQuit();

    cout.rdbuf(old);
    CHECK(buffer.str().find("goodbye") != string::npos);
}

// =============================================================
// UIDisclaimer::show  —  exit via quit / back / menu
// =============================================================

TEST_CASE("UIDisclaimer::show — quit exits cleanly") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();
    ConfigManager cfg(configPath);

    istringstream input("quit\n");
    streambuf* oldCin  = cin.rdbuf(input.rdbuf());

    stringstream output;
    streambuf* oldCout = cout.rdbuf(output.rdbuf());

    bool result = UIDisclaimer::show(cfg);

    cout.rdbuf(oldCout);
    cin.rdbuf(oldCin);

    CHECK(result == false);
    CHECK(output.str().find("goodbye") != string::npos);
}

TEST_CASE("UIDisclaimer::show — back exits cleanly") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();
    ConfigManager cfg(configPath);

    istringstream input("back\n");
    streambuf* oldCin  = cin.rdbuf(input.rdbuf());

    stringstream output;
    streambuf* oldCout = cout.rdbuf(output.rdbuf());

    bool result = UIDisclaimer::show(cfg);

    cout.rdbuf(oldCout);
    cin.rdbuf(oldCin);

    CHECK(result == false);
}

TEST_CASE("UIDisclaimer::show — menu exits cleanly") {
    test_fixtures::TempDirectory dir;
    auto configPath = (dir.path / "config.json").string();
    ConfigManager cfg(configPath);

    istringstream input("menu\n");
    streambuf* oldCin  = cin.rdbuf(input.rdbuf());

    stringstream output;
    streambuf* oldCout = cout.rdbuf(output.rdbuf());

    bool result = UIDisclaimer::show(cfg);

    cout.rdbuf(oldCout);
    cin.rdbuf(oldCin);

    CHECK(result == false);
}

// =============================================================
// UI_attention  —  quit inside attention prompt
// =============================================================

TEST_CASE("UI_attention — quit returns 0 and prints goodbye") {
    istringstream input("quit\n");
    streambuf* oldCin  = cin.rdbuf(input.rdbuf());

    stringstream output;
    streambuf* oldCout = cout.rdbuf(output.rdbuf());

    char result = UI_attention("test message");

    cout.rdbuf(oldCout);
    cin.rdbuf(oldCin);

    CHECK(result == 0);
    CHECK(output.str().find("goodbye") != string::npos);
}
