/*
 *  tguide — UI_input.h
 *  global input handler — every screen uses this instead of cin directly
 *
 *  written by voidoxin
 */

#pragma once
#include <string>
#include <vector>

// ==================== NORMALIZE ====================

// trim leading/trailing whitespace and lowercase — mirrors UI_disclaimer normalize
std::string normalize(const std::string& input);

// ==================== INPUT ====================

// print prompt with green color, read one line from cin, trim whitespace
// returns empty string on EOF
std::string readInput(const std::string& prompt) noexcept;

// ==================== CHECKS ====================

// returns true if input is "0" or "back" (case-insensitive)
bool isBack(const std::string& input);

// returns true if input is "q", "quit", or "exit" (case-insensitive)
bool isQuit(const std::string& input);

// returns true if input is "n" or "next" (case-insensitive)
bool isNext(const std::string& input);

// returns true if input is "p" or "prev" (case-insensitive)
bool isPrev(const std::string& input);

// ==================== NUMBER ====================

// returns integer value if input is a pure digit string, -1 otherwise
// "0" returns 0 — caller must check isBack before calling toNumber
int toNumber(const std::string& input);

// ==================== MATCH ====================

// match input against a list of option labels — priority: number → exact name → prefix
// returns 0-based index on match, -1 on no match or ambiguous prefix
int matchOption(const std::string& input, const std::vector<std::string>& options);

// ==================== QUIT ====================

// clear screen and print goodbye message — does NOT call exit()
void handleQuit();