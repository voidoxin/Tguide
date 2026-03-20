/*
 *  tguide — UI_savedCommands.h
 *  written by voidoxin
 *
 *  Saved Commands screen — personal command library.
 *  Displays user-saved commands, allows add / edit / delete / copy.
 *  Will connect to svc_savedCommands → L0-core user command storage.
 */

#pragma once

namespace UISavedCommands {

    // render the Saved Commands screen — blocks until user navigates back
    void show();

}