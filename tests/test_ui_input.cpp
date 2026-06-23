#include "doctest.h"
#include "UI_input.h"
#include <string>
#include <vector>

using namespace std;

// =============================================================
// matchOption  —  Main Menu label matching
// =============================================================

static const vector<string> mainMenuLabels = {
    "Tools",
    "Saved Commands",
    "Saved Scripts",
    "Settings",
    "Exit"
};

TEST_CASE("matchOption — number matching") {
    // "1" with labels → index 0 (Tools)
    CHECK(matchOption("1", mainMenuLabels) == 0);
    CHECK(matchOption("2", mainMenuLabels) == 1);
    CHECK(matchOption("3", mainMenuLabels) == 2);
    CHECK(matchOption("4", mainMenuLabels) == 3);
    CHECK(matchOption("5", mainMenuLabels) == 4);
}

TEST_CASE("matchOption — exact match (case-insensitive)") {
    // "tools" → index 0
    CHECK(matchOption("tools", mainMenuLabels) == 0);
}

TEST_CASE("matchOption — exact match mixed case") {
    // "Tools" → index 0
    CHECK(matchOption("Tools", mainMenuLabels) == 0);
}

TEST_CASE("matchOption — exact match all caps") {
    // "SETTINGS" → index 3
    CHECK(matchOption("SETTINGS", mainMenuLabels) == 3);
}

TEST_CASE("matchOption — prefix match (unambiguous)") {
    // "se" uniquely matches "Settings" (index 3)
    CHECK(matchOption("se", mainMenuLabels) == 3);
    // "ex" uniquely matches "Exit" (index 4)
    CHECK(matchOption("ex", mainMenuLabels) == 4);
    // "to" uniquely matches "Tools" (index 0)
    CHECK(matchOption("to", mainMenuLabels) == 0);
}

TEST_CASE("matchOption — prefix match (unambiguous) partial word") {
    // "saved c" uniquely matches "Saved Commands" (index 1)
    CHECK(matchOption("saved c", mainMenuLabels) == 1);
    // "saved s" uniquely matches "Saved Scripts" (index 2)
    CHECK(matchOption("saved s", mainMenuLabels) == 2);
}

TEST_CASE("matchOption — prefix match (ambiguous)") {
    // "s" matches both "Saved Commands" and "Saved Scripts" → -1
    CHECK(matchOption("s", mainMenuLabels) == -1);
    // "sa" also ambiguous
    CHECK(matchOption("sa", mainMenuLabels) == -1);
    // "sav" also ambiguous
    CHECK(matchOption("sav", mainMenuLabels) == -1);
    // "saved" also ambiguous
    CHECK(matchOption("saved", mainMenuLabels) == -1);
}

TEST_CASE("matchOption — no match") {
    // "xyz" does not match any label → -1
    CHECK(matchOption("xyz", mainMenuLabels) == -1);
}

TEST_CASE("matchOption — empty input") {
    // "" → -1
    CHECK(matchOption("", mainMenuLabels) == -1);
}

TEST_CASE("matchOption — whitespace-only input") {
    // "   " normalizes to empty → -1
    CHECK(matchOption("   ", mainMenuLabels) == -1);
}

TEST_CASE("matchOption — number 0") {
    // "0" → -1 (matchOption only accepts 1-based numbers)
    CHECK(matchOption("0", mainMenuLabels) == -1);
}

TEST_CASE("matchOption — quit keywords not matched") {
    // "quit" should not match any main menu label
    CHECK(matchOption("quit", mainMenuLabels) == -1);
    // "q" also should not match
    CHECK(matchOption("q", mainMenuLabels) == -1);
    // "exit" is a label, so it SHOULD match (index 4)
    // This is actually an exact match for "Exit"
    CHECK(matchOption("exit", mainMenuLabels) == 4);
}

TEST_CASE("matchOption — empty options list") {
    vector<string> empty;
    CHECK(matchOption("1", empty) == -1);
    CHECK(matchOption("anything", empty) == -1);
}

TEST_CASE("matchOption — single element list") {
    vector<string> single = {"Only Option"};
    CHECK(matchOption("1", single) == 0);
    CHECK(matchOption("only", single) == 0);
    CHECK(matchOption("o", single) == 0);   // unambiguous prefix
    CHECK(matchOption("x", single) == -1);  // no match
}

TEST_CASE("matchOption — numbers beyond range") {
    // "6" out of range → -1
    CHECK(matchOption("6", mainMenuLabels) == -1);
    // "99" also out of range
    CHECK(matchOption("99", mainMenuLabels) == -1);
    // negative numbers not valid input → -1
    CHECK(matchOption("-1", mainMenuLabels) == -1);
}

TEST_CASE("matchOption — mixed digit/non-digit input") {
    // "1abc" is not a pure number → not matched by number rule
    // It should fall through to exact/prefix match
    // None of the labels match "1abc" → -1
    CHECK(matchOption("1abc", mainMenuLabels) == -1);
}

// =============================================================
// toNumber  —  helper used by matchOption internally
// =============================================================

TEST_CASE("toNumber — basic digits") {
    CHECK(toNumber("1") == 1);
    CHECK(toNumber("5") == 5);
    CHECK(toNumber("123") == 123);
}

TEST_CASE("toNumber — zero") {
    CHECK(toNumber("0") == 0);
}

TEST_CASE("toNumber — invalid input returns -1") {
    CHECK(toNumber("") == -1);
    CHECK(toNumber("abc") == -1);
    CHECK(toNumber("1a") == -1);
    CHECK(toNumber("-1") == -1);
    CHECK(toNumber(" 1") == -1);    // spaces not stripped
}

// =============================================================
// normalize  —  used by matchOption internally
// =============================================================

TEST_CASE("normalize — trims and lowercases") {
    CHECK(normalize("  Hello  ") == "hello");
    CHECK(normalize("ABC") == "abc");
    CHECK(normalize("  Mixed CASE  ") == "mixed case");
}

TEST_CASE("normalize — empty and whitespace") {
    CHECK(normalize("") == "");
    CHECK(normalize("   ") == "");
    CHECK(normalize("\t\n\r") == "");
}

TEST_CASE("normalize — control chars stripped") {
    // null char (non-printable) should be stripped
    // NOTE: \x01" "c splits hex escape from 'c' to avoid \x01c being parsed as \x1c
    string withCtrl = "ab\x01" "c";
    CHECK(normalize(withCtrl) == "abc");
}
