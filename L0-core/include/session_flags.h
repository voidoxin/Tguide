#pragma once
// Session-only flags set by CLI arguments.
// These override config or normal behavior for the current session only.
// Defined in session_flags.cpp.

extern bool g_quietMode;    // --quiet: suppress non-critical output
extern bool g_verboseMode;  // --verbose: print detailed operation trace
extern bool g_offlineMode;  // --offline: prevent network requests
extern bool g_noBanner;     // --no-banner: suppress startup banner
extern bool g_stream;       // --stream / -S: disable pagination for this session
