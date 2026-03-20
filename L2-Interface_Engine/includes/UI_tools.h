/*
 *  tguide — UI_tools.h
 *  written by voidoxin
 *
 *  Tools screen — entry point for browsing the security tools reference.
 *  Displays tool list, flags, templates, and usage examples.
 *  Will connect to svc_tools → L0-core ToolD / ToolFlagD / TemplateD queries.
 */

#pragma once

namespace UITools {

    // render the Tools screen — blocks until user navigates back
    void show();
