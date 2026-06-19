#pragma once

#include <iosfwd>
#include <string>


// Stores the result of parsing command-line arguments.
// New flags will be added here in STEP-20, STEP-21, etc.
struct ParsedArgs {
    bool help    = false;  // --help / -h
    bool version = false;  // --version / -v
    bool yes     = false;  // --yes / -y

    // Output Control (STEP-20)
    bool quiet     = false;  // --quiet / -q
    bool verbose   = false;  // --verbose / -V
    bool noColor   = false;  // --no-color
    bool noBanner  = false;  // --no-banner
    bool offline   = false;  // --offline
    bool stream    = false;  // --stream / -S  (wired fully in STEP-26)

    // Configuration (STEP-20)
    std::string setArg;       // --set <setting>=<value>
    std::string resetArg;     // --reset <setting>|all
    bool ignoreConfig = false; // --ignore-config
    bool cacheClear   = false; // --cache-clear

    // Display (STEP-21)
    std::string toolArg;          // --tool / -t <names>  (comma-separated)
    bool flagsFilter   = false;   // --flags / -f
    bool descFilter    = false;   // --description / -d
    bool templatesFilter = false; // --templates / -temp
    std::string filterArg;        // --filter / -F <field>=<value,...>
    bool vuln           = false;  // --vuln / -vl
    std::string categoryArg;      // --category / -c <name>

    // Saved Data & Export (STEP-22)
    bool savedScripts = false;          // --saved-scripts / -sc
    std::string exportTextArg;          // --export-text <file_path>
    std::string exportJsonArg;          // --export-json <file_path>
    std::string exportYamlArg;          // --export-yaml <file_path>
    std::string exportCsvArg;           // --export-csv <file_path>

    // Update (STEP-23)
    bool checkUpdate = false;  // --check-update
    bool update = false;       // --update

    // Log queries (STEP-25)
    bool logView = false;              // --log
    bool logLastBoot = false;          // --log-b
    int logBootOffset = -1;            // --log-b-1, --log-b-2, ... (N >= 1)
    std::string logDateArg;            // --log-date <YYYY-MM-DD>

    std::string error;     // empty = no error; non-empty = error message to show
};

// Parse argc/argv and return structured results.
// On unknown flags: sets args.error with a descriptive message.
// On --help or --version: sets the corresponding bool, does NOT exit.
ParsedArgs parseArgs(int argc, char* argv[]);

// Print full usage information.
// Lists all known flags with their short forms and descriptions.
// Uses plain output (no ANSI colors — that's L2 layer's job).
// The caller can pass std::cerr for the error-usage path.
void printUsage(std::ostream& os);

// Print version information.
// Prints program version (TGUIDE_VERSION compile-time define).
// Database version will be added when manifest-caching is improved.
// Example output: "tguide v1.0.1"
void printVersion(std::ostream& os);
