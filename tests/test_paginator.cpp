/*
 *  tguide — test_paginator.cpp
 *  Tests for CLI output paginator
 *
 *  written by voidoxin
 */

#include "doctest.h"
#include "paginator.h"
#include "session_flags.h"
#include <sstream>
#include <iostream>

// Test paginator output by capturing stdout
static std::string capturePaginate(const std::string& output, int pageSize, bool disabled) {
    std::stringstream buffer;
    std::streambuf* old = std::cout.rdbuf(buffer.rdbuf());

    // Simulate stdin input for pagination prompts
    // We'll test streaming mode (disabled=true) to avoid blocking on stdin
    Paginator::paginate(output, pageSize, disabled);

    std::cout.rdbuf(old);
    return buffer.str();
}

TEST_CASE("Paginator — streaming mode outputs everything") {
    std::string input = "line1\nline2\nline3\n";
    std::string result = capturePaginate(input, 1, true);
    CHECK(result.find("line1") != std::string::npos);
    CHECK(result.find("line2") != std::string::npos);
    CHECK(result.find("line3") != std::string::npos);
    CHECK(result.find("More") == std::string::npos);  // no pagination prompt
}

TEST_CASE("Paginator — empty output produces no output") {
    std::string result = capturePaginate("", 5, true);
    CHECK(result.empty());
}

TEST_CASE("Paginator — page size 0 disables pagination") {
    std::string input = "a\nb\nc\n";
    std::string result = capturePaginate(input, 0, false);
    CHECK(result.find("a") != std::string::npos);
    CHECK(result.find("b") != std::string::npos);
    CHECK(result.find("c") != std::string::npos);
    CHECK(result.find("More") == std::string::npos);
}

TEST_CASE("Paginator — single line with small page size") {
    // With disabled=true we don't test the interactive prompt
    std::string result = capturePaginate("hello world", 1, true);
    CHECK(result.find("hello world") != std::string::npos);
}

TEST_CASE("Paginator — single line without trailing newline") {
    std::string result = capturePaginate("no newline at end", 5, true);
    CHECK(result.find("no newline at end") != std::string::npos);
}

// Helper: capture paginator output with simulated stdin input
static std::string capturePaginateWithInput(const std::string& output,
                                             int pageSize,
                                             const std::string& stdinInput) {
    // Redirect cout to capture output
    std::stringstream outBuffer;
    std::streambuf* oldCout = std::cout.rdbuf(outBuffer.rdbuf());

    // Redirect cin to provide simulated input
    std::istringstream inBuffer(stdinInput);
    std::streambuf* oldCin = std::cin.rdbuf(inBuffer.rdbuf());

    Paginator::paginate(output, pageSize, false);

    // Restore streams
    std::cout.rdbuf(oldCout);
    std::cin.rdbuf(oldCin);

    return outBuffer.str();
}

TEST_CASE("Paginator — quit on q during interactive pagination") {
    // 4 lines, pageSize=2, simulate pressing 'q' at the prompt
    std::string result = capturePaginateWithInput("line1\nline2\nline3\nline4\n", 2, "q\n");

    // First page (2 lines) should appear
    CHECK(result.find("line1") != std::string::npos);
    CHECK(result.find("line2") != std::string::npos);

    // Pagination prompt should be shown
    CHECK(result.find("More") != std::string::npos);

    // Line 3 should NOT appear (we quit at the prompt)
    CHECK(result.find("line3") == std::string::npos);
}

TEST_CASE("Paginator — Enter advances to next page") {
    // 5 lines, pageSize=2 → two prompts (after page 1 and page 2), 5th line on page 3
    // Simulate Enter to advance, then Enter again, then q to quit on third page
    std::string result = capturePaginateWithInput("line1\nline2\nline3\nline4\nline5\n", 2, "\n\nq\n");

    // All lines should appear
    CHECK(result.find("line1") != std::string::npos);
    CHECK(result.find("line2") != std::string::npos);
    CHECK(result.find("line3") != std::string::npos);
    CHECK(result.find("line4") != std::string::npos);
    CHECK(result.find("line5") != std::string::npos);

    // Pagination prompt should appear twice (after page 1 and page 2)
    size_t firstMore = result.find("More");
    size_t secondMore = result.find("More", firstMore + 1);
    CHECK(firstMore != std::string::npos);
    CHECK(secondMore != std::string::npos);
}

TEST_CASE("Paginator — exact page boundary does not prompt") {
    // 2 lines, pageSize=2 — exact fit, no prompt should appear
    std::string result = capturePaginateWithInput("line1\nline2\n", 2, "");

    CHECK(result.find("line1") != std::string::npos);
    CHECK(result.find("line2") != std::string::npos);
    CHECK(result.find("More") == std::string::npos);
}
