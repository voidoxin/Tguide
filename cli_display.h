#pragma once
#include "L0-core/include/cli_parser.h"
#include <string>

// Run the appropriate display command based on parsed CLI args.
// Queries the database, formats output, prints to stdout, then returns.
// Should be called after bootstrap (config + DB ready), before UI start.
// Returns 0 on success, 1 on error.
int runDisplayCommand(const ParsedArgs& args, const std::string& dbPath);
