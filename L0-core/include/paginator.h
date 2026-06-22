/*
 *  tguide — paginator.h
 *  CLI output paginator
 *
 *  written by voidoxin
 */

#pragma once

#include <string>

// CLI output paginator: prints a multi-line string page-by-page.
// Each page shows N lines, then a "-- More --" prompt.
// Enter = next page, q = quit.
namespace Paginator {

    // Print output with pagination.
    // If paginationDisabled is true, prints all output at once with no pauses.
    void paginate(const std::string& output, int pageSize, bool paginationDisabled);

} // namespace Paginator
