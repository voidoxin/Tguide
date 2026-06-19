#include "doctest.h"
#include "cli_parser.h"
#include <sstream>

TEST_CASE("parseArgs — --help (long)") {
    const char* argv[] = {"tguide", "--help", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.help == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — -h (short)") {
    const char* argv[] = {"tguide", "-h", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.help == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --version") {
    const char* argv[] = {"tguide", "--version", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.version == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — -v") {
    const char* argv[] = {"tguide", "-v", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.version == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --yes") {
    const char* argv[] = {"tguide", "--yes", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.yes == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — -y") {
    const char* argv[] = {"tguide", "-y", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.yes == true);
    CHECK(args.error.empty());
}

//
// Output Control (STEP-20)
//

TEST_CASE("parseArgs — --quiet") {
    const char* argv[] = {"tguide", "--quiet", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.quiet == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — -q") {
    const char* argv[] = {"tguide", "-q", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.quiet == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --verbose") {
    const char* argv[] = {"tguide", "--verbose", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.verbose == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — -V") {
    const char* argv[] = {"tguide", "-V", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.verbose == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --no-color") {
    const char* argv[] = {"tguide", "--no-color", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.noColor == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --no-banner") {
    const char* argv[] = {"tguide", "--no-banner", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.noBanner == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --offline") {
    const char* argv[] = {"tguide", "--offline", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.offline == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --stream") {
    const char* argv[] = {"tguide", "--stream", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.stream == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — -S") {
    const char* argv[] = {"tguide", "-S", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.stream == true);
    CHECK(args.error.empty());
}

//
// Configuration (STEP-20)
//

TEST_CASE("parseArgs — --set with argument") {
    const char* argv[] = {"tguide", "--set", "colors=off", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.setArg == "colors=off");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs -- --set without argument produces error") {
    const char* argv[] = {"tguide", "--set", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("--set requires") != std::string::npos);
}

TEST_CASE("parseArgs — --reset with argument") {
    const char* argv[] = {"tguide", "--reset", "all", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.resetArg == "all");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --reset without argument produces error") {
    const char* argv[] = {"tguide", "--reset", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("--reset requires") != std::string::npos);
}

TEST_CASE("parseArgs — --ignore-config") {
    const char* argv[] = {"tguide", "--ignore-config", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.ignoreConfig == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --cache-clear") {
    const char* argv[] = {"tguide", "--cache-clear", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.cacheClear == true);
    CHECK(args.error.empty());
}

//
// Combined flags
//

TEST_CASE("parseArgs — --set with --verbose combined") {
    const char* argv[] = {"tguide", "--set", "colors=off", "--verbose", nullptr};
    auto args = parseArgs(4, const_cast<char**>(argv));
    CHECK(args.setArg == "colors=off");
    CHECK(args.verbose == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --quiet --no-color --offline combined") {
    const char* argv[] = {"tguide", "--quiet", "--no-color", "--offline", nullptr};
    auto args = parseArgs(4, const_cast<char**>(argv));
    CHECK(args.quiet == true);
    CHECK(args.noColor == true);
    CHECK(args.offline == true);
    CHECK(args.error.empty());
}

//
// Error cases
//

TEST_CASE("parseArgs — unknown flag produces error") {
    const char* argv[] = {"tguide", "--bogus-flag", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("Unknown option") != std::string::npos);
}

TEST_CASE("parseArgs — unknown short flag produces error") {
    const char* argv[] = {"tguide", "-Z", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("Unknown option") != std::string::npos);
}

//
// printUsage / printVersion — basic smoke tests
//

TEST_CASE("printUsage — outputs usage text") {
    std::ostringstream oss;
    printUsage(oss);
    CHECK(oss.str().size() > 100);
    CHECK(oss.str().find("Usage: tguide") != std::string::npos);
    CHECK(oss.str().find("--quiet") != std::string::npos);
    CHECK(oss.str().find("--no-color") != std::string::npos);
    CHECK(oss.str().find("--set") != std::string::npos);
    CHECK(oss.str().find("--reset") != std::string::npos);
    CHECK(oss.str().find("--cache-clear") != std::string::npos);
    CHECK(oss.str().find("--ignore-config") != std::string::npos);
    CHECK(oss.str().find("--offline") != std::string::npos);
    CHECK(oss.str().find("--stream") != std::string::npos);
}

TEST_CASE("printVersion — outputs version string") {
    std::ostringstream oss;
    printVersion(oss);
    CHECK(oss.str().find("tguide v") != std::string::npos);
}

//
// Saved Data & Export (STEP-22)
//

TEST_CASE("parseArgs — --saved-scripts (long)") {
    const char* argv[] = {"tguide", "--saved-scripts", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.savedScripts == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — -sc (multi-char short)") {
    const char* argv[] = {"tguide", "-sc", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.savedScripts == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --export-text with path") {
    const char* argv[] = {"tguide", "--export-text", "/tmp/out.txt", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.exportTextArg == "/tmp/out.txt");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --export-text without path produces error") {
    const char* argv[] = {"tguide", "--export-text", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("--export-text requires") != std::string::npos);
}

TEST_CASE("parseArgs — --export-json with path") {
    const char* argv[] = {"tguide", "--export-json", "/tmp/out.json", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.exportJsonArg == "/tmp/out.json");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --export-yaml with path") {
    const char* argv[] = {"tguide", "--export-yaml", "/tmp/out.yaml", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.exportYamlArg == "/tmp/out.yaml");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --export-csv with path") {
    const char* argv[] = {"tguide", "--export-csv", "/tmp/out.csv", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.exportCsvArg == "/tmp/out.csv");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --export-json without path produces error") {
    const char* argv[] = {"tguide", "--export-json", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("--export-json requires") != std::string::npos);
}

TEST_CASE("parseArgs — --export-yaml without path produces error") {
    const char* argv[] = {"tguide", "--export-yaml", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("--export-yaml requires") != std::string::npos);
}

TEST_CASE("parseArgs — --export-csv without path produces error") {
    const char* argv[] = {"tguide", "--export-csv", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("--export-csv requires") != std::string::npos);
}

TEST_CASE("parseArgs — --saved-scripts combined with export") {
    const char* argv[] = {"tguide", "--saved-scripts", "--export-text", "/tmp/out.txt", nullptr};
    auto args = parseArgs(4, const_cast<char**>(argv));
    CHECK(args.savedScripts == true);
    CHECK(args.exportTextArg == "/tmp/out.txt");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --tool combined with --export-text") {
    const char* argv[] = {"tguide", "--tool", "nmap", "--export-text", "/tmp/out.txt", nullptr};
    auto args = parseArgs(5, const_cast<char**>(argv));
    CHECK(args.toolArg == "nmap");
    CHECK(args.exportTextArg == "/tmp/out.txt");
    CHECK(args.error.empty());
}

//
// Update flags (STEP-23)
//

TEST_CASE("parseArgs — --check-update") {
    const char* argv[] = {"tguide", "--check-update", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.checkUpdate == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --update") {
    const char* argv[] = {"tguide", "--update", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.update == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --update --yes combined") {
    const char* argv[] = {"tguide", "--update", "--yes", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.update == true);
    CHECK(args.yes == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --check-update --quiet combined") {
    const char* argv[] = {"tguide", "--check-update", "--quiet", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.checkUpdate == true);
    CHECK(args.quiet == true);
    CHECK(args.error.empty());
}

//
// Log query flags (STEP-25)
//

TEST_CASE("parseArgs — --log (long form)") {
    const char* argv[] = {"tguide", "--log", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.logView == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --log-b") {
    const char* argv[] = {"tguide", "--log-b", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.logLastBoot == true);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --log-b-1 (N=1)") {
    const char* argv[] = {"tguide", "--log-b-1", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.logBootOffset == 1);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --log-b-3 (N=3)") {
    const char* argv[] = {"tguide", "--log-b-3", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK(args.logBootOffset == 3);
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --log-date with valid date") {
    const char* argv[] = {"tguide", "--log-date", "2026-06-19", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.logDateArg == "2026-06-19");
    CHECK(args.error.empty());
}

TEST_CASE("parseArgs — --log-date without argument produces error") {
    const char* argv[] = {"tguide", "--log-date", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("--log-date requires") != std::string::npos);
}

TEST_CASE("parseArgs — --log-b- (no number → error)") {
    const char* argv[] = {"tguide", "--log-b-", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("Missing number after --log-b-") != std::string::npos);
}

TEST_CASE("parseArgs — --log-b-abc (non-numeric → invalid format → error)") {
    const char* argv[] = {"tguide", "--log-b-abc", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("Invalid --log-b-N format") != std::string::npos);
}

TEST_CASE("parseArgs — --log-b-0 (N < 1 → error)") {
    const char* argv[] = {"tguide", "--log-b-0", nullptr};
    auto args = parseArgs(2, const_cast<char**>(argv));
    CHECK_FALSE(args.error.empty());
    CHECK(args.error.find("N >= 1") != std::string::npos);
}

TEST_CASE("parseArgs — --log combined with --log-b") {
    // Both should be set — useful for testing combined flags
    const char* argv[] = {"tguide", "--log", "--log-b", nullptr};
    auto args = parseArgs(3, const_cast<char**>(argv));
    CHECK(args.logView == true);
    CHECK(args.logLastBoot == true);
    CHECK(args.error.empty());
}

TEST_CASE("printUsage shows log flags") {
    std::ostringstream oss;
    printUsage(oss);
    CHECK(oss.str().find("--log") != std::string::npos);
    CHECK(oss.str().find("--log-b") != std::string::npos);
    CHECK(oss.str().find("--log-b-<N>") != std::string::npos);
    CHECK(oss.str().find("--log-date") != std::string::npos);
}
