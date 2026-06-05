/*
 *  tguide — svc_generator.h
 *  written by voidoxin
 *
 *  Service layer for the Script Generator screen.
 *  Handles all business logic between UI_generator and L0-core:
 *    - load available tools and their flags for the builder
 *    - resolve flag dependencies and mutual exclusions
 *    - assemble the final command string from user selections
 *    - save generated commands/scripts to user storage
 *  Implementation pending.
 */

#pragma once
#include <string>

// strips shell metacharacters from user input; safe to use from any layer
// see: .ai/security.md — sanitizeInput rule
std::string sanitizeInput(const std::string& input);

namespace SvcGenerator {

    // placeholder — implementation pending

}