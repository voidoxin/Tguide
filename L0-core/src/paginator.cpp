/*
 *  tguide — paginator.cpp
 *  CLI output paginator implementation
 *
 *  written by voidoxin
 */

#include "../include/paginator.h"
#include <iostream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

namespace Paginator {

void paginate(const std::string& output, int pageSize, bool paginationDisabled) {
    if (output.empty())
        return;

    // Streaming mode: print everything at once
    if (paginationDisabled || pageSize <= 0) {
        std::cout << output;
        return;
    }

    // Split output by newlines
    std::vector<std::string> lines;
    std::istringstream iss(output);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    if (lines.empty())
        return;

    size_t pos = 0;
    while (pos < lines.size()) {
        size_t end = std::min(pos + static_cast<size_t>(pageSize), lines.size());
        for (size_t i = pos; i < end; ++i) {
            std::cout << lines[i] << '\n';
        }
        pos = end;

        if (pos >= lines.size())
            break;

        size_t remaining = lines.size() - pos;
        std::cout << "\n-- More (" << remaining << " remaining) -- "
                  << "Press Enter, q + Enter to quit: ";
        std::string input;
        if (!std::getline(std::cin, input)) {
            // EOF or error on stdin — exit
            break;
        }
        if (!input.empty()) {
            char c = std::tolower(static_cast<unsigned char>(input[0]));
            if (c == 'q' || c == 'x')
                break;
        }
        // Empty line or Enter = continue
    }
}

} // namespace Paginator
