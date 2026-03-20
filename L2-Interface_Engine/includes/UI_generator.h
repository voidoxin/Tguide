/*
 *  tguide — UI_generator.h
 *  written by voidoxin
 *
 *  Script Generator screen — interactive command builder.
 *  Lets the user select a tool, fill in parameters, and produce
 *  a ready-to-run command or shell script.
 *  Will connect to svc_generator → L0-core ToolFlagD / TemplateD queries.
 */

#pragma once

namespace UIGenerator {

    // render the Script Generator screen — blocks until user navigates back
    void show();

}