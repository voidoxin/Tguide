/*
 *  tguide — UI_errorHandling.h
 *  written by voidoxin
 */

#pragma once
#include <string>

// waits for user input before continuing — used after errors
void pause();

// non-recoverable error: prints message, waits for enter
// caller is responsible for exiting after this
void UI_fatal(const std::string& msg);

// recoverable error: prints message, execution continues
void UI_errors(const std::string& msg);