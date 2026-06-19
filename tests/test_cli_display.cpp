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
#include "UserDataManager.h"
#include "logger.h"
#include "svc_savedScripts.h"
#include "fixtures.h"
#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <filesystem>
#include "libs/json.hpp"

using json = nlohmann::json;
using namespace std;
namespace fs = std::filesystem;

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

//
// --saved-scripts tests (STEP-22)
// These tests initialize UserDataManager with temp paths.
//

TEST_CASE("--saved-scripts with no scripts shows empty message") {
    test_fixtures::TempDirectory tmpDir;
    auto scriptsPath = (tmpDir.path / "scripts.json").string();
    auto cmdsPath = (tmpDir.path / "commands.json").string();

    // Init empty UserDataManager
    UserDataManager::instance().init(cmdsPath, scriptsPath);
    UserDataManager::instance().load();

    ParsedArgs args;
    args.savedScripts = true;

    string output = captureDisplay(args);
    CHECK(output.find("No saved scripts found") != string::npos);
}

TEST_CASE("--saved-scripts displays saved scripts") {
    test_fixtures::TempDirectory tmpDir;
    auto scriptsPath = (tmpDir.path / "scripts.json").string();
    auto cmdsPath = (tmpDir.path / "commands.json").string();

    UserDataManager::instance().init(cmdsPath, scriptsPath);
    UserDataManager::instance().load();

    // Save a test script
    UserDataManager::instance().saveScript("test.sh", "/home/test.sh", "A test script");

    ParsedArgs args;
    args.savedScripts = true;

    string output = captureDisplay(args);
    CHECK(output.find("test.sh") != string::npos);
    CHECK(output.find("/home/test.sh") != string::npos);
    CHECK(output.find("A test script") != string::npos);
}

TEST_CASE("--saved-scripts --export-text writes file") {
    test_fixtures::TempDirectory tmpDir;
    auto scriptsPath = (tmpDir.path / "scripts.json").string();
    auto cmdsPath = (tmpDir.path / "commands.json").string();
    auto exportPath = (tmpDir.path / "export.txt").string();

    UserDataManager::instance().init(cmdsPath, scriptsPath);
    UserDataManager::instance().load();
    UserDataManager::instance().saveScript("myscript.sh", "/opt/myscript.sh", "My script");

    ParsedArgs args;
    args.savedScripts = true;
    args.exportTextArg = exportPath;

    // captureDisplay also triggers export
    string output = captureDisplay(args);

    // Check file was written
    CHECK(std::filesystem::exists(exportPath));
    string fileContent;
    {
        ifstream f(exportPath);
        fileContent = string((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    }
    CHECK(fileContent.find("myscript.sh") != string::npos);
    CHECK(fileContent.find("/opt/myscript.sh") != string::npos);
}

TEST_CASE("--saved-scripts --export-json writes valid JSON") {
    test_fixtures::TempDirectory tmpDir;
    auto scriptsPath = (tmpDir.path / "scripts.json").string();
    auto cmdsPath = (tmpDir.path / "commands.json").string();
    auto exportPath = (tmpDir.path / "export.json").string();

    UserDataManager::instance().init(cmdsPath, scriptsPath);
    UserDataManager::instance().load();
    UserDataManager::instance().saveScript("script1", "/path/a", "note a");
    UserDataManager::instance().saveScript("script2", "/path/b", "note b");

    ParsedArgs args;
    args.savedScripts = true;
    args.exportJsonArg = exportPath;

    captureDisplay(args);

    CHECK(std::filesystem::exists(exportPath));
    // Verify it's valid JSON by parsing with nlohmann/json
    ifstream f(exportPath);
    string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    auto parsed = json::parse(content);  // throws on invalid JSON
    CHECK(parsed.contains("scripts"));
    CHECK(parsed["scripts"].is_array());
    CHECK(parsed["scripts"].size() == 2);
    CHECK(parsed["scripts"][0]["name"] == "script1");
    CHECK(parsed["scripts"][1]["name"] == "script2");
}

TEST_CASE("--saved-scripts --export-yaml writes YAML") {
    test_fixtures::TempDirectory tmpDir;
    auto scriptsPath = (tmpDir.path / "scripts.json").string();
    auto cmdsPath = (tmpDir.path / "commands.json").string();
    auto exportPath = (tmpDir.path / "export.yaml").string();

    UserDataManager::instance().init(cmdsPath, scriptsPath);
    UserDataManager::instance().load();
    UserDataManager::instance().saveScript("test", "/t", "desc");

    ParsedArgs args;
    args.savedScripts = true;
    args.exportYamlArg = exportPath;

    captureDisplay(args);

    CHECK(std::filesystem::exists(exportPath));
    ifstream f(exportPath);
    string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    CHECK(content.find("scripts:") != string::npos);
    CHECK(content.find("test") != string::npos);
}

TEST_CASE("--saved-scripts --export-csv writes CSV") {
    test_fixtures::TempDirectory tmpDir;
    auto scriptsPath = (tmpDir.path / "scripts.json").string();
    auto cmdsPath = (tmpDir.path / "commands.json").string();
    auto exportPath = (tmpDir.path / "export.csv").string();

    UserDataManager::instance().init(cmdsPath, scriptsPath);
    UserDataManager::instance().load();
    UserDataManager::instance().saveScript("a", "/a", "note");

    ParsedArgs args;
    args.savedScripts = true;
    args.exportCsvArg = exportPath;

    captureDisplay(args);

    CHECK(std::filesystem::exists(exportPath));
    ifstream f(exportPath);
    string content((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    CHECK(content.find("id,name,path,note") != string::npos);
    bool foundSimple = content.find("a,/a,note") != string::npos;
    bool foundQuoted = content.find("a,\"/a\",\"note\"") != string::npos;
    CHECK((foundSimple || foundQuoted));
}

TEST_CASE("export without base command shows error") {
    ParsedArgs args;
    args.exportTextArg = "/tmp/some_file.txt";

    int result = runDisplayCommand(args, seedDbPath());
    CHECK(result == 1);  // Error expected
}

TEST_CASE("--tool nmap --export-text writes file") {
    test_fixtures::TempDirectory tmpDir;
    auto exportPath = (tmpDir.path / "tool_output.txt").string();

    ParsedArgs args;
    args.toolArg = "nmap";
    args.exportTextArg = exportPath;

    string output = captureDisplay(args);
    CHECK(output.find("=== nmap ===") != string::npos);
    CHECK(std::filesystem::exists(exportPath));

    ifstream f(exportPath);
    string fileContent((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    CHECK(fileContent.find("=== nmap ===") != string::npos);
}

TEST_CASE("--vuln --export-text writes file") {
    test_fixtures::TempDirectory tmpDir;
    auto exportPath = (tmpDir.path / "vuln_output.txt").string();

    ParsedArgs args;
    args.vuln = true;
    args.exportTextArg = exportPath;

    string output = captureDisplay(args);
    CHECK(output.find("EternalBlue") != string::npos);
    CHECK(std::filesystem::exists(exportPath));

    ifstream f(exportPath);
    string fileContent((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    CHECK(fileContent.find("EternalBlue") != string::npos);
}

TEST_CASE("--category --export-text writes file") {
    test_fixtures::TempDirectory tmpDir;
    auto exportPath = (tmpDir.path / "cat_output.txt").string();

    ParsedArgs args;
    args.categoryArg = "Network Scanning";
    args.exportTextArg = exportPath;

    string output = captureDisplay(args);
    CHECK(output.find("nmap") != string::npos);
    CHECK(std::filesystem::exists(exportPath));

    ifstream f(exportPath);
    string fileContent((istreambuf_iterator<char>(f)), istreambuf_iterator<char>());
    CHECK(fileContent.find("nmap") != string::npos);
}

//
// Update command tests (STEP-23)
//

TEST_CASE("runUpdateCommand --update without db returns error") {
    // Test error path when database doesn't exist
    ParsedArgs args;
    args.update = true;

    // Non-existent path
    int result = runUpdateCommand(args, "/nonexistent/path/tguide.db",
                                   "/nonexistent/path/.db_cache");
    CHECK(result == 1);  // Error expected
}

TEST_CASE("runUpdateCommand --check-update without network returns error") {
    // Test error path when manifest fetch fails (no network)
    ParsedArgs args;
    args.checkUpdate = true;

    int result = runUpdateCommand(args, "/nonexistent/path/tguide.db",
                                   "/nonexistent/path/.db_cache");
    CHECK(result == 1);  // Error expected — no network
}

TEST_CASE("printUsage shows update flags") {
    // Verify update flags appear in usage output
    std::ostringstream oss;
    printUsage(oss);
    CHECK(oss.str().find("--update") != std::string::npos);
    CHECK(oss.str().find("--check-update") != std::string::npos);
}

//
// Log query command tests (STEP-25)
//

TEST_CASE("runLogCommand — --log shows all entries") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());

    // Write some entries
    Logger::instance().info("test entry one");
    Logger::instance().warning("test entry two");

    // Build args
    ParsedArgs args;
    args.logView = true;

    // Capture stdout
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    runLogCommand(args);

    std::cout.rdbuf(old);
    std::string output = buffer.str();

    CHECK(output.find("test entry one") != std::string::npos);
    CHECK(output.find("test entry two") != std::string::npos);
}

TEST_CASE("runLogCommand — --log-b shows last boot entries") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Create a raw log file with entries from two different boots.
    // We write the file manually to simulate two boot sessions.
    {
        std::ofstream f(logPath.string());
        // Boot 1 entries
        f << "2026-06-19 10:00:00 [INFO] [boot=1] first boot: startup\n";
        f << "2026-06-19 10:01:00 [ERROR] [boot=1] first boot: something failed\n";
        // Boot 2 entries
        f << "2026-06-19 11:00:00 [INFO] [boot=2] second boot: started\n";
        f << "2026-06-19 11:01:00 [WARNING] [boot=2] second boot: warning\n";
    }

    // Init logger — will scan file, find maxBootId=2, set current to 3
    Logger::instance().init(logPath.string());

    // Also write a new entry for the current boot (boot 3)
    Logger::instance().info("current boot entry");

    ParsedArgs args;
    args.logLastBoot = true;

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    runLogCommand(args);

    std::cout.rdbuf(old);
    std::string output = buffer.str();

    // Should contain boot 2 entries (current boot is 3, so last boot is 2)
    CHECK(output.find("second boot: started") != std::string::npos);
    CHECK(output.find("second boot: warning") != std::string::npos);
    // Should NOT contain boot 1 or boot 3 entries
    CHECK(output.find("first boot:") == std::string::npos);
    CHECK(output.find("current boot entry") == std::string::npos);
}

TEST_CASE("runLogCommand — --log-b-1 shows boot before last") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Create raw log with three boots
    {
        std::ofstream f(logPath.string());
        f << "2026-06-19 10:00:00 [INFO] [boot=1] boot 1 entry\n";
        f << "2026-06-19 11:00:00 [INFO] [boot=2] boot 2 entry\n";
        f << "2026-06-19 12:00:00 [INFO] [boot=3] boot 3 entry\n";
    }

    Logger::instance().init(logPath.string());
    // current boot = 4

    ParsedArgs args;
    args.logBootOffset = 1;
    // target = current - 1 - 1 = 4 - 1 - 1 = 2

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    runLogCommand(args);

    std::cout.rdbuf(old);
    std::string output = buffer.str();

    CHECK(output.find("boot 2 entry") != std::string::npos);
    CHECK(output.find("boot 1 entry") == std::string::npos);
    CHECK(output.find("boot 3 entry") == std::string::npos);
}

TEST_CASE("runLogCommand — --log-date filters entries by date") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    // Create raw log with entries on different dates
    {
        std::ofstream f(logPath.string());
        f << "2026-06-18 10:00:00 [INFO] [boot=1] yesterday entry\n";
        f << "2026-06-19 10:00:00 [INFO] [boot=1] today entry one\n";
        f << "2026-06-19 11:00:00 [INFO] [boot=1] today entry two\n";
        f << "2026-06-20 10:00:00 [INFO] [boot=1] tomorrow entry\n";
    }

    Logger::instance().init(logPath.string());

    ParsedArgs args;
    args.logDateArg = "2026-06-19";

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    runLogCommand(args);

    std::cout.rdbuf(old);
    std::string output = buffer.str();

    CHECK(output.find("today entry one") != std::string::npos);
    CHECK(output.find("today entry two") != std::string::npos);
    CHECK(output.find("yesterday entry") == std::string::npos);
    CHECK(output.find("tomorrow entry") == std::string::npos);
}

TEST_CASE("runLogCommand — --log-date with no matches shows empty message") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    {
        std::ofstream f(logPath.string());
        f << "2026-06-19 10:00:00 [INFO] [boot=1] some entry\n";
    }

    Logger::instance().init(logPath.string());

    ParsedArgs args;
    args.logDateArg = "2025-01-01";

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    runLogCommand(args);

    std::cout.rdbuf(old);
    std::string output = buffer.str();

    CHECK(output.find("no matching log entries") != std::string::npos);
}

TEST_CASE("runLogCommand — uninitialized logger returns error") {
    // Reset the singleton so we can test the uninitialized path
    Logger::instance().resetForTesting();
    CHECK_FALSE(Logger::instance().isInitialized());

    ParsedArgs args;
    args.logView = true;

    // Capture stderr
    std::stringstream buffer;
    std::streambuf* old = std::cerr.rdbuf(buffer.rdbuf());

    int result = runLogCommand(args);

    std::cerr.rdbuf(old);
    std::string output = buffer.str();

    CHECK(result == 1);
    CHECK(output.find("Logger not initialized") != std::string::npos);
}

TEST_CASE("runLogCommand — --log-b with single boot shows all entries") {
    test_fixtures::TempDirectory dir;
    fs::path logPath = dir.path / "tguide.log";

    Logger::instance().init(logPath.string());
    Logger::instance().info("only boot entry");

    ParsedArgs args;
    args.logLastBoot = true;

    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    runLogCommand(args);

    std::cout.rdbuf(old);
    std::string output = buffer.str();

    // With a single boot, --log-b should show its entries
    CHECK(output.find("only boot entry") != std::string::npos);
}
