#pragma once
#include "L0-core/include/cli_parser.h"
#include <string>

// Run the appropriate display command based on parsed CLI args.
// Queries the database, formats output, prints to stdout, then returns.
// Should be called after bootstrap (config + DB ready), before UI start.
// Returns 0 on success, 1 on error.
int runDisplayCommand(const ParsedArgs& args, const std::string& dbPath);

// Run update CLI commands (--check-update, --update).
// Requires DBCacheManager to be initialized (cachePath).
// dbPath is the resolved database path.
// Returns 0 on success, 1 on error.
int runUpdateCommand(const ParsedArgs& args,
                     const std::string& dbPath,
                     const std::string& cachePath);

// Run log query commands (--log, --log-b, --log-b-N, --log-date).
// Opens an interactive viewer with pagination.
// Requires Logger to be initialized.
// Returns 0 on success, 1 on error.
int runLogCommand(const ParsedArgs& args);
