#pragma once

#include <iosfwd>
#include <string>


// Stores the result of parsing command-line arguments.
// New flags will be added here in STEP-20, STEP-21, etc.
struct ParsedArgs {
    bool help    = false;  // --help / -h
    bool version = false;  // --version / -v
    bool yes     = false;  // --yes / -y
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
