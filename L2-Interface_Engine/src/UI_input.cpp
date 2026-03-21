/*
 *  tguide — UI_input.cpp
 *  global input handler implementation
 *
 *  written by voidoxin
 */

#include "../includes/UI_input.h"
#include "../includes/UI_colors.h"
#include "../includes/UI_utils.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <stdexcept>

using namespace std;

// ==================== NORMALIZE ====================

// trim leading/trailing whitespace and lowercase — identical logic to UI_disclaimer
string normalize(const string& input) {
    string out = input;
    size_t start = out.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    out = out.substr(start);
    size_t end = out.find_last_not_of(" \t\r\n");
    if (end != string::npos) out = out.substr(0, end + 1);
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// ==================== INPUT ====================

// print prompt in green, read one line, trim whitespace, return empty on EOF
string readInput(const string& prompt) {
    cout << (colorsEnabled() ? Color::GREEN : "")
         << prompt
         << (colorsEnabled() ? Color::RESET : "");

    string line;
    if (!getline(cin, line)) return "";

    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == string::npos) return "";
    size_t end = line.find_last_not_of(" \t\r\n");
    return line.substr(start, end - start + 1);
}

// ==================== CHECKS ====================

// returns true if input normalizes to "0" or "back"
bool isBack(const string& input) {
    string n = normalize(input);
    return n == "0" || n == "back";
}

// returns true if input normalizes to "q", "quit", or "exit"
bool isQuit(const string& input) {
    string n = normalize(input);
    return n == "q" || n == "quit" || n == "exit";
}

// returns true if input normalizes to "n" or "next"
bool isNext(const string& input) {
    string n = normalize(input);
    return n == "n" || n == "next";
}

// returns true if input normalizes to "p" or "prev"
bool isPrev(const string& input) {
    string n = normalize(input);
    return n == "p" || n == "prev";
}

// ==================== NUMBER ====================

// returns the integer value for a pure digit string, -1 on any non-digit or overflow
int toNumber(const string& input) {
    if (input.empty()) return -1;
    for (char c : input) {
        if (!isdigit(static_cast<unsigned char>(c))) return -1;
    }
    try {
        return stoi(input);
    } catch (...) {
        return -1;
    }
}

// ==================== MATCH ====================

// match input against option labels with three-level priority:
//   1. exact number (1-based) → 0-based index
//   2. exact name (case-insensitive)
//   3. prefix match — returns -1 if ambiguous (multiple matches)
int matchOption(const string& input, const vector<string>& options) {
    string norm = normalize(input);
    if (norm.empty()) return -1;

    // 1. number match (1-based)
    int n = toNumber(norm);
    if (n >= 1 && n <= static_cast<int>(options.size())) return n - 1;

    // 2. exact name match
    for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        if (normalize(options[i]) == norm) return i;
    }

    // 3. prefix match — ambiguous if more than one candidate found
    int match = -1;
    for (int i = 0; i < static_cast<int>(options.size()); ++i) {
        string opt = normalize(options[i]);
        if (opt.size() >= norm.size() && opt.compare(0, norm.size(), norm) == 0) {
            if (match != -1) return -1;
            match = i;
        }
    }
    return match;
}

// ==================== QUIT ====================

// clear screen and print goodbye — caller is responsible for returning
void handleQuit() {
    UI::clearScreen();
    cout << (colorsEnabled() ? Color::CYAN  : "")
         << (colorsEnabled() ? Color::BOLD  : "")
         << "\n  goodbye.\n\n"
         << (colorsEnabled() ? Color::RESET : "");
}