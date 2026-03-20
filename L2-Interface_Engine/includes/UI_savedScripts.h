/*
 *  tguide — UI_savedScripts.h
 *  written by voidoxin
 *
 *  Saved Scripts screen — personal script library.
 *  Displays user-saved generated scripts, allows view / edit / delete / export.
 *  Will connect to svc_savedScripts → L0-core user script storage.
 */

#pragma once

namespace UISavedScripts {

    // render the Saved Scripts screen — blocks until user navigates back
    void show();

}