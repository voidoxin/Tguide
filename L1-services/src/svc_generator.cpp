/*
 *  tguide — svc_generator.cpp
 *  written by voidoxin
 *
 *  Service layer for the Script Generator screen.
 *  Connects UI_generator → L0-core ToolFlagD / TemplateD queries.
 *  Implementation pending.
 */

#include "../includes/svc_generator.h"
#include <string>

// strips shell metacharacters from user input to prevent command injection
// see: .ai/security.md — sanitizeInput rule
std::string sanitizeInput(const std::string& input) {
    std::string result;
    result.reserve(input.size());
    for (char c : input) {
        if (c == ' ' || c == ';' || c == '|' || c == '&' ||
            c == '$' || c == '>' || c == '<' ||
            c == '`' || c == '\n' || c == '\r' ||
            c == '\'' || c == '"' || c == '\\' ||
            c == '(' || c == ')' || c == '{' || c == '}' ||
            c == '!' || c == '~' || c == '*' || c == '?' ||
            c == '[' || c == '#')
            continue;
        result += c;
    }
    return result;
}