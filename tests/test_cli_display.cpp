/*
 *  tguide — test_cli_display.cpp
 *  Tests for CLI display commands (--tool, --vuln, --category)
 *
 *  written by voidoxin
 */

#include "doctest.h"
#include "cli_parser.h"
#include "cli_display.h"
#include "DatabaseManager.h"
#include "fixtures.h"
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

// Helper: open the seed database for direct query
static string seedDbPath() {
    return string(TGUIDE_SOURCE_DIR) + "/data/database/tguide.db";
}

// Helper: capture stdout from a call to runDisplayCommand
static string captureDisplay(const ParsedArgs& args) {
    stringstream buffer;
    streambuf* old = cout.rdbuf(buffer.rdbuf());

    runDisplayCommand(args, seedDbPath());

    cout.rdbuf(old);
    return buffer.str();
}

// ==============================================================
// --tool tests
// ==============================================================

TEST_CASE("--tool 'nmap' produces output with tool name") {
    ParsedArgs args;
    args.toolArg = "nmap";

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("Description:") != string::npos);
    CHECK(output.find("Flags:") != string::npos);
    CHECK(output.find("Templates:") != string::npos);
}

TEST_CASE("--tool with multiple names") {
    ParsedArgs args;
    args.toolArg = "nmap,hydra";

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("=== hydra ===") != string::npos);
}

TEST_CASE("--tool with nonexistent name prints not-found message") {
    ParsedArgs args;
    args.toolArg = "nonexistent_tool_xyz";

    string output = captureDisplay(args);
    CHECK(output.find("not found") != string::npos);
}

TEST_CASE("--tool with fuzzy-close name suggests match") {
    ParsedArgs args;
    args.toolArg = "nmap";   // exact match — no fuzzy needed

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);

    // Test fuzzy suggestion with a close typo
    ParsedArgs fuzzyArgs;
    fuzzyArgs.toolArg = "namp";  // edit distance 1 from "nmap"
    string fuzzyOutput = captureDisplay(fuzzyArgs);
    CHECK(fuzzyOutput.find("nmap") != string::npos);
}

TEST_CASE("--tool --flags shows only flags") {
    ParsedArgs args;
    args.toolArg = "nmap";
    args.flagsFilter = true;

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("Flags:") != string::npos);
    CHECK(output.find("-sS") != string::npos);
    // Description should NOT be present when only --flags is specified
    CHECK(output.find("Description:") == string::npos);
}

TEST_CASE("--tool --description shows only description") {
    ParsedArgs args;
    args.toolArg = "nmap";
    args.descFilter = true;

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("Description:") != string::npos);
    CHECK(output.find("Flags:") == string::npos);
}

TEST_CASE("--tool --templates shows only templates") {
    ParsedArgs args;
    args.toolArg = "nmap";
    args.templatesFilter = true;

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("Templates:") != string::npos);
    CHECK(output.find("Flags:") == string::npos);
    CHECK(output.find("Description:") == string::npos);
}

TEST_CASE("--tool --flags --templates shows both sections") {
    ParsedArgs args;
    args.toolArg = "nmap";
    args.flagsFilter = true;
    args.templatesFilter = true;

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("Flags:") != string::npos);
    CHECK(output.find("Templates:") != string::npos);
    CHECK(output.find("Description:") == string::npos);
}

// ==============================================================
// --vuln tests
// ==============================================================

TEST_CASE("--vuln produces output with vulnerability info") {
    ParsedArgs args;
    args.vuln = true;

    string output = captureDisplay(args);
    CHECK(output.find("EternalBlue") != string::npos);
    CHECK(output.find("Severity:") != string::npos);
}

TEST_CASE("--vuln --filter severity=critical filters vulns") {
    ParsedArgs args;
    args.vuln = true;
    args.filterArg = "severity=critical";

    string output = captureDisplay(args);
    CHECK(output.find("EternalBlue") != string::npos);
    CHECK(output.find("Severity: critical") != string::npos);
}

TEST_CASE("--vuln --filter platform=windows filters by platform") {
    ParsedArgs args;
    args.vuln = true;
    args.filterArg = "platform=windows";

    string output = captureDisplay(args);
    CHECK(output.find("Platform: windows") != string::npos);
}

// ==============================================================
// --category tests
// ==============================================================

TEST_CASE("--category 'Network Scanning' shows tools in that category") {
    ParsedArgs args;
    args.categoryArg = "Network Scanning";

    string output = captureDisplay(args);
    CHECK(output.find("=== Network Scanning ===") != string::npos);
    CHECK(output.find("nmap") != string::npos);
    CHECK(output.find("netcat") != string::npos);
}

TEST_CASE("--category with unknown category shows no-tools message") {
    ParsedArgs args;
    args.categoryArg = "NonexistentCategory";

    string output = captureDisplay(args);
    CHECK(output.find("No tools found") != string::npos);
}

// ==============================================================
// Edge case tests
// ==============================================================

TEST_CASE("empty display args returns 0 without output") {
    ParsedArgs args;

    string output = captureDisplay(args);
    CHECK(output.empty());
}

TEST_CASE("--tool with spaces around comma-separated names") {
    ParsedArgs args;
    args.toolArg = "  nmap  ,  sqlmap  ";

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("=== sqlmap ===") != string::npos);
}

TEST_CASE("--vuln with invalid filter field shows all vulns") {
    ParsedArgs args;
    args.vuln = true;
    args.filterArg = "bogus_field=whatever";

    string output = captureDisplay(args);
    // Should still show vulns with a warning
    CHECK(output.find("EternalBlue") != string::npos);
}

TEST_CASE("--tool with filter by category") {
    ParsedArgs args;
    args.toolArg = "nmap,sqlmap";
    args.filterArg = "category=Network Scanning";

    string output = captureDisplay(args);
    // nmap is in Network Scanning, sqlmap is not
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(output.find("=== sqlmap ===") == string::npos);
}
