/*
 *  tguide — UI_errorHandling.h
 *  fatal / recoverable error display and acknowledgement primitives
 *
 *  written by voidoxin
 */

#pragma once
#include <string>

// ==================== PAUSE ====================

// waits for user to press enter — use when the screen must not advance yet
// prints "  press enter to continue..." then blocks on readInput
void waitForEnter() noexcept;

// ==================== FATAL ====================

// non-recoverable error: prints [FATAL] message, waits for enter, then returns
// prints "  press enter to exit..." — NOT "continue"
// NEVER call clearScreen() after this — the terminal output is the error record
// caller is responsible for returning 1 from main()
void UI_fatal(const std::string& msg) noexcept;

// ==================== RECOVERABLE ====================

// recoverable error: prints [error] message and waits for acknowledgement
// waitForEnter is built in — callers do NOT need a separate waitForEnter() call after this
// do NOT call clearScreen() immediately after UI_errors() in the same call frame
void UI_errors(const std::string& msg);

// ==================== ATTENTION ====================

// prints [!] message and reads one char response (e.g. y/n prompt)
// returns the char entered, or 0 on EOF or read failure
// clears remaining input from the buffer after reading
char UI_attention(const std::string& msg);

// ==================== AUDIT ====================
//
// Error-handling contract:     VERIFIED CLEAN (2026-06-07)
//
// Rules enforced at every call site:
//   Rule 1 — UI_fatal()  → NEVER followed by clearScreen()
//                            (caller returns 1 immediately after)
//   Rule 2 — UI_errors() → has waitForEnter() built in;
//                            callers do NOT add clearScreen()
//                            in the same call frame
//   Rule 3 — waitForEnter() → blocks until user presses enter;
//                              used both internally (UI_fatal,
//                              UI_errors) and as standalone
//                              informational pauses
//
// For detailed call-site breakdown, see:
//   UI_errorHandling.h (this file) — per-function comments
//   UI_Engine.cpp / CoreRunner.cpp — contract rule enforcement
