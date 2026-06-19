#include "cli_parser.h"
#include <iostream>

// ==============================================================
// Flag Definition Table
// ==============================================================
// Each entry describes one known flag.  short_name is 0 when there
// is no single-character short form.  Multi-character short forms
// (e.g. -temp for --templates) are listed with their own long_name
// entries so the parser can find them when the user types -temp.
//
// Order here determines the order in which flags are displayed
// in printUsage().
// ==============================================================
struct FlagDef {
    const char*  long_name;
    const char   short_name;   // 0 = no short form
    const char*  description;
};

static const FlagDef known_flags[] = {
    // ── General ────────────────────────────────────────────────
    {"--help",             'h',   "Print usage information and exit."},
    {"--version",          'v',   "Print program version and exit."},
    {"--yes",              'y',   "Auto-approve interactive prompts."},

    // ── Output Control ─────────────────────────────────────────
    {"--no-color",         0,     "Disable colored output for this session."},
    {"--quiet",            'q',   "Suppress non-critical output."},
    {"--verbose",          'V',   "Print detailed operation trace."},
    {"--no-banner",        0,     "Suppress startup banner."},
    {"--offline",          0,     "Force offline mode (no network requests)."},
    {"--stream",           'S',   "Disable pagination for this session."},

    // ── Configuration ──────────────────────────────────────────
    {"--set",              0,     "Permanently modify a setting: --set <key>=<value>"},
    {"--reset",            0,     "Reset setting(s) to defaults: --reset <key>|all"},
    {"--ignore-config",    0,     "Run with factory defaults."},
    {"--cache-clear",      0,     "Clear cached data and exit."},

    // ── Display ────────────────────────────────────────────────
    {"--tool",             't',   "Print tool information from database."},
    {"--flags",            'f',   "Filter: show only flags for a tool."},
    {"--description",      'd',   "Filter: show only description for a tool."},
    {"--templates",        0,     "Filter: show only templates for a tool."},
    {"--temp",             0,     "Alias for --templates."},
    {"--filter",           'F',   "Filter criteria (field=value)."},
    {"--vuln",             0,     "Display vulnerabilities."},
    {"--vl",               0,     "Alias for --vuln."},
    {"--category",         'c',   "Show tools in a category."},

    // ── Saved Data & Export ────────────────────────────────────
    {"--saved-scripts",    0,     "Display saved scripts."},
    {"--sc",               0,     "Alias for --saved-scripts."},
    {"--export-text",      0,     "Export output to text file."},
    {"--export-json",      0,     "Export output to JSON file."},
    {"--export-yaml",      0,     "Export output to YAML file."},
    {"--export-csv",       0,     "Export output to CSV file."},

    // ── Update ─────────────────────────────────────────────────
    {"--update",           0,     "Check for and download updates."},
    {"--check-update",     0,     "Check for updates without downloading."},

    // ── Log ────────────────────────────────────────────────────
    {"--log",              0,     "Display full program log (interactive viewer)."},
    {"--log-b",            0,     "Show last boot's errors."},
    {"--log-date",         0,     "Show log entries for a specific date."},
};

// ==============================================================
// parseArgs
// ==============================================================
ParsedArgs parseArgs(int argc, char* argv[]) {
    ParsedArgs args;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        const FlagDef* matched = nullptr;

        // ── Try long-form match: --xxx ──────────────────────────
        if (arg.size() >= 2 && arg[0] == '-' && arg[1] == '-') {
            for (const auto& flag : known_flags) {
                if (arg == flag.long_name) {
                    matched = &flag;
                    break;
                }
            }
        }
        // ── Try short-form match: -X (exactly 2 chars) ─────────
        else if (arg.size() == 2 && arg[0] == '-') {
            const char c = arg[1];
            for (const auto& flag : known_flags) {
                if (flag.short_name == c) {
                    matched = &flag;
                    break;
                }
            }
        }
        // ── Multi-char short form (e.g. -temp, -vl, -sc) ───────
        // Convert -xxx to --xxx and look up in long-name table.
        else if (arg.size() > 2 && arg[0] == '-') {
            const std::string as_long = "--" + arg.substr(1);
            for (const auto& flag : known_flags) {
                if (as_long == flag.long_name) {
                    matched = &flag;
                    break;
                }
            }
        }

        // ── Not found → unknown option ─────────────────────────
        if (!matched) {
            args.error = "Unknown option: " + arg;
            break;
        }

        // ── Route to the appropriate handler ────────────────────
        const std::string name(matched->long_name);

        if (name == "--help") {
            args.help = true;
        } else if (name == "--version") {
            args.version = true;
        } else if (name == "--yes") {
            args.yes = true;
        } else if (name == "--set") {
            if (i + 1 >= argc) {
                args.error = "--set requires an argument, e.g. --set colors=off";
                break;
            }
            args.setArg = argv[++i];
        } else if (name == "--reset") {
            if (i + 1 >= argc) {
                args.error = "--reset requires an argument, e.g. --reset colors or --reset all";
                break;
            }
            args.resetArg = argv[++i];
        } else if (name == "--quiet") {
            args.quiet = true;
        } else if (name == "--verbose") {
            args.verbose = true;
        } else if (name == "--no-color") {
            args.noColor = true;
        } else if (name == "--no-banner") {
            args.noBanner = true;
        } else if (name == "--offline") {
            args.offline = true;
        } else if (name == "--ignore-config") {
            args.ignoreConfig = true;
        } else if (name == "--cache-clear") {
            args.cacheClear = true;
        } else if (name == "--stream") {
            args.stream = true;
        } else if (name == "--tool") {
            if (i + 1 >= argc) {
                args.error = "--tool requires an argument, e.g. --tool nmap";
                break;
            }
            args.toolArg = argv[++i];
        } else if (name == "--flags") {
            args.flagsFilter = true;
        } else if (name == "--description") {
            args.descFilter = true;
        } else if (name == "--templates" || name == "--temp") {
            args.templatesFilter = true;
        } else if (name == "--filter") {
            if (i + 1 >= argc) {
                args.error = "--filter requires an argument, e.g. --filter severity=high";
                break;
            }
            args.filterArg = argv[++i];
        } else if (name == "--vuln" || name == "--vl") {
            args.vuln = true;
        } else if (name == "--category") {
            if (i + 1 >= argc) {
                args.error = "--category requires an argument, e.g. --category recon";
                break;
            }
            args.categoryArg = argv[++i];
        } else {
            args.error = std::string("Option '") + arg + "' is not yet implemented";
            break;
        }
    }

    return args;
}

// ==============================================================
// printUsage
// ==============================================================
void printUsage(std::ostream& os) {
    os << "Usage: tguide [OPTIONS]\n"
              << "\n"
              << "General:\n"
              << "  -h, --help       Print usage information and exit.\n"
              << "  -v, --version    Print program version and exit.\n"
              << "  -y, --yes        Auto-approve interactive prompts.\n"
              << "\n"
              << "Output Control:\n"
              << "  --no-color       Disable colored output for this session.\n"
              << "  -q, --quiet      Suppress non-critical output.\n"
              << "  -V, --verbose    Print detailed operation trace.\n"
              << "  --no-banner      Suppress startup banner.\n"
              << "  --offline        Force offline mode (no network requests).\n"
              << "  -S, --stream     Disable pagination for this session.\n"
              << "\n"
              << "Configuration:\n"
              << "  --set <key>=<value>   Permanently modify a setting.\n"
              << "  --reset <key>|all     Reset setting(s) to defaults.\n"
              << "  --ignore-config       Run with factory defaults.\n"
              << "  --cache-clear         Clear cached data and exit.\n"
              << "\n"
              << "Display:\n"
              << "  -t, --tool       Print tool information from database.\n"
              << "  -f, --flags      Filter: show only flags for a tool.\n"
              << "  -d, --description Filter: show only description for a tool.\n"
              << "  -temp, --templates Filter: show only templates for a tool.\n"
              << "  -F, --filter     Filter criteria (field=value).\n"
              << "  -vl, --vuln      Display vulnerabilities.\n"
              << "  -c, --category   Show tools in a category.\n"
              << "\n"
              << "Saved Data & Export:\n"
              << "  -sc, --saved-scripts  Display saved scripts.\n"
              << "  --export-text    Export output to text file.\n"
              << "  --export-json    Export output to JSON file.\n"
              << "  --export-yaml    Export output to YAML file.\n"
              << "  --export-csv     Export output to CSV file.\n"
              << "\n"
              << "Update:\n"
              << "  --update         Check for and download updates.\n"
              << "  --check-update   Check for updates without downloading.\n"
              << "\n"
              << "Log:\n"
              << "  --log            Display full program log (interactive viewer).\n"
              << "  --log-b          Show last boot's errors.\n"
              << "  --log-b-<N>      Show log entries N boots ago.\n"
              << "  --log-date       Show log entries for a specific date.\n"
              << std::flush;
}

// ==============================================================
// printVersion
// ==============================================================
void printVersion(std::ostream& os) {
    os << "tguide v" << TGUIDE_VERSION << std::endl;
}
