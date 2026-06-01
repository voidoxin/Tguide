# tguide — Development Plan
# written by voidoxin
# last updated: 2026-03-20

================================================================
DECISIONS & ASSUMPTIONS (locked)
================================================================

DB-1:  categories → single TEXT column inside tools table
       query fetches distinct values then filters by chosen one

DB-2:  tools table needs TWO description columns:
         short_desc  TEXT  — one sentence, shown under tool name in lists
         description TEXT  — full detailed explanation, shown in tool detail view
       current single `description` column maps to the new `description`
       new `short_desc` column must be added

DB-3:  templates table — add `description` TEXT column now (Task 0)

DB-4:  user data stored in TWO separate JSON files (no SQLite, no root required)
       json.hpp already in project — zero new dependencies

       saved_commands.json:  { "commands": [ { "id", "tool_id", "command", "note" } ] }
       saved_scripts.json:   { "scripts":  [ { "id", "name", "path", "note" } ] }

       paths per platform:
         Linux   → ~/.local/share/tguide/saved_commands.json
                   ~/.local/share/tguide/saved_scripts.json
                   ~/.local/share/tguide/scripts/
         Termux  → $PREFIX/share/tguide/saved_commands.json
                   $PREFIX/share/tguide/saved_scripts.json
                   $PREFIX/share/tguide/scripts/
         Windows → %APPDATA%\tguide\saved_commands.json
                   %APPDATA%\tguide\saved_scripts.json
                   %APPDATA%\tguide\scripts\
         macOS   → ~/Library/Application Support/tguide/saved_commands.json
                   ~/Library/Application Support/tguide/saved_scripts.json
                   ~/Library/Application Support/tguide/scripts/

       managed by UserDataManager (pattern similar to ConfigManager)
       PathResolver must expose: userDataDir(), savedCommandsFile(),
                                 savedScriptsFile(), scriptsDir()

DB-5:  saved_commands fields:
         id       — auto-incremented int (managed in JSON)
         tool_id  — references tools.id in tguide.db
         command  — full built command string
         note     — one-line description

DB-6:  saved_scripts fields:
         id    — auto-incremented int (managed in JSON)
         name  — script filename without extension
         path  — absolute path to generated script file
         note  — one-line description

NAV-1: navigation keywords accepted everywhere at any prompt:
         0 or "back"        → go to previous screen
         "q", "quit", "exit" → close program immediately (clean exit)
       these are checked BEFORE any other input processing

NAV-2: pagination for long lists (tools, flags, templates):
         show 10 items per page
         accepted keywords: "next" or "n" → next page
                            "prev" or "p" → previous page
                            number        → select item
                            "back" or 0   → exit list
         current page indicator shown: [page 1/3]
         screen cleared on every page turn

FEAT-1: search screen — UI fully built, accepts input,
        returns mock/empty results for now
        algorithm implementation is a future task (marked TODO)

FEAT-2: filters — available filters decided at implementation time
        placeholder in plan for now

FEAT-3: template display — always prompts user to fill missing values
        supported placeholders: <target_ip>, <target_domain>,
                                 <target_email>, <target_phone>,
                                 <port>, <wordlist>, <output_file>
        after filling → show final command in a clear copyable block
        user can then choose to save it

FEAT-4: Script Generator — user picks multiple templates,
        fills in values for each, output is a .sh bash script
        scripts saved to a fixed internal directory:
          Linux/Termux → ~/.local/share/tguide/scripts/
          Windows      → %APPDATA%\tguide\scripts\
        run from inside tguide is planned (future task)

FEAT-5: legal disclaimer on first run
        text written by voidoxin (see LEGAL section below)
        acceptance stored in config.json as "disclaimer_accepted": 1
        if declined → show again, program does not proceed
        if accepted → saved via ConfigManager, never shown again


FEAT-7: metasploit special sub-menu — Vulnerabilities
        when tool name == "metasploit" (case-insensitive), add extra option F
        in the tool detail screen: "Vulnerabilities"
        sub-menu options:
          1. Search       — accepts input, algorithm TODO, returns empty for now
          2. Filter       — filters: severity, service, platform, access
          3. Show All     — paginated list from vulnerabilities + options tables
        each vulnerability shows: name, metasploit_name, severity, access,
        platform, service, discovered_date, discoverer, description, danger, options

FEAT-8: recon-ng special sub-menu — Modules
        when tool name == "recon-ng" (case-insensitive), add extra option F
        in the tool detail screen: "Modules"
        sub-menu options:
          1. Search       — accepts input, algorithm TODO, returns empty for now
          2. Filter       — filters: type, platform, mode (active/passive), loud
          3. Show All     — paginated list from modules table
        each module shows: name, path, platform, type, description, API, mode, loud, output

FEAT-9: free navigation — input accepts names not just numbers
        at every menu/list prompt, user can type:
          - a number → select by index
          - an option name or partial name → select by name (case-insensitive)
        this applies to main menu, tools menu, category list, tool list,
        tool detail options, and all sub-menus
        implemented in UI_input::matchOption() helper

FEAT-6: settings — color toggle
        config.json gets new key:
          "colors": 1    → ANSI colors enabled
          "colors": 0    → plain output, no colors
        UI_colors.h reads this at runtime (not compile-time)
        environment detection still overrides on Windows (forced off)

================================================================
LEGAL DISCLAIMER TEXT
================================================================

  TGUIDE — LEGAL DISCLAIMER

  This tool is intended solely for authorized security testing,
  educational purposes, and lawful penetration testing activities.

  By using tguide, you confirm that:
    - You have explicit written permission to test the target systems.
    - You are not using this tool for unauthorized access, surveillance,
      or any activity that violates local, national, or international law.
    - You accept full responsibility for any actions performed using
      this tool.

  The author (voidoxin) provides this software "as is" without warranty
  of any kind and bears no liability for damages, legal consequences,
  or misuse arising from the use of this tool.

  Unauthorized use of this tool against systems you do not own or have
  permission to test is illegal and punishable by law.

  Type "agree" to accept and continue, or "exit" to quit.


================================================================
ARCHITECTURAL FIX — Linux path strategy (no root required)
================================================================

PROBLEM:
  The original design placed config in /etc/tguide/ and DB in
  /usr/share/tguide/ — both require root on Linux. Any user
  running tguide without sudo hits UI_fatal immediately.
  This contradicts the goal of a tool that works for any user.

DECISION:
  ALL runtime paths move to user space on every platform.
  System paths (/etc/, /usr/share/) are only written by the
  package manager at install time — never by the running app.

NEW PATH STRATEGY:

  Linux (no package manager):
    config   → ~/.config/tguide/config.json
    DB       → ~/.local/share/tguide/tguide.db
    user data→ ~/.local/share/tguide/saved_commands.json etc.

  Linux (installed via .deb):
    package manager writes DB to /usr/share/tguide/tguide.db
    on first run: app copies DB to ~/.local/share/tguide/
    all subsequent reads from ~/.local/share/tguide/
    config always in ~/.config/tguide/

  Termux:
    config   → $PREFIX/etc/tguide/config.json
    DB       → $PREFIX/share/tguide/tguide.db
    user data→ $PREFIX/share/tguide/

  Windows:
    everything → %APPDATA%\tguide\

  macOS:
    everything → ~/Library/Application Support/tguide/

CHANGES REQUIRED (pending implementation):

  path_resolver.h:
    - configDir() on Linux → ~/.config/tguide/
    - dataDir() on Linux   → ~/.local/share/tguide/
    - remove the /etc/ and /usr/share/ Linux paths entirely
    - hasWriteAccess() → always return true (user space is always writable)
    - createSystemDirs() and createUserDirs() merge into one createDirs()
      since there is no longer a privilege distinction

  CoreRunner.cpp:
    - remove hasWriteAccess() check (no longer needed)
    - replace createSystemDirs()/createUserDirs() calls with single createDirs()
    - remove the fatal error for directory creation failure
      (user space dirs failing is non-fatal — app still runs)

  CMakeLists.txt install targets:
    - Linux install: binary → /usr/bin/tguide (still needs root for install)
    - Linux install: DB     → /usr/share/tguide/tguide.db (read by app on first run)
    - Linux install: NO config install — app creates it on first run
    - resolveDatabase() Case 3a handles the copy from /usr/share/ to ~/.local/share/

  NOTE: the .deb install still requires sudo for `cmake --install`
  but running the installed tool never requires root.


================================================================
DB DESIGN ADDITION — categories table
================================================================

PROBLEM:
  Categories are currently derived from a TEXT column in tools
  (SELECT DISTINCT category). This means:
    - no control over display order
    - no way to rename a category without touching every tool
    - no way to add a category before adding tools to it

SOLUTION: dedicated categories table

  Schema:
    CREATE TABLE IF NOT EXISTS categories (
      id          INTEGER PRIMARY KEY AUTOINCREMENT,
      name        TEXT NOT NULL UNIQUE,
      display_order INTEGER DEFAULT 0,
      description TEXT
    )

  tools.category column now stores the category NAME (same as before)
  but the source of truth for order and display is the categories table.

  Query pattern for UI:
    SELECT name FROM categories ORDER BY display_order ASC, name ASC
  → gives the ordered list for the category browser

  tools still use category TEXT — no foreign key constraint
  (keeps data_adder simple, avoids cascade issues on rename)

DATABASE CHANGES:

  DatabaseManager.h — add CategoryD class:

    struct Category {
        int         id;
        std::string name;
        int         display_order;
        std::string description;
    };

    class CategoryD {
    public:
        CategoryD(const std::string& path);
        bool createTables();
        bool add(const Category& c);
        bool del(int id);
        bool update(const Category& c);         // rename + reorder
        bool reorder(int id, int new_order);    // change position only
        std::vector<Category> getAll();         // sorted by display_order
        std::vector<Category> getOrdered();     // same — semantic alias
    };

  DatabaseManager.cpp — implement CategoryD

  data_adder.cpp — add category management menu:
    - list all categories
    - add category
    - rename category (updates all tools.category via UPDATE tools SET category=? WHERE category=?)
    - delete category (warns if tools still assigned)
    - reorder categories (swap display_order values)
    NOT in CoreRunner.cpp, NOT in any UI screen

  svc_tools.cpp — getCategories() updated:
    instead of SELECT DISTINCT, query CategoryD::getOrdered()
    then return vector<string> of names in display_order sequence

DYNAMIC TOOL-TO-CATEGORY ASSIGNMENT:

  From data_adder, when adding a tool:
    - list all categories from CategoryD::getAll()
    - user picks category by number or types a new one
    - if new: auto-insert into categories table with next display_order
    - tool.category = chosen name

  Renaming a category in data_adder propagates automatically:
    UPDATE tools SET category = ? WHERE category = ?

  This means the developer (voidoxin or contributor) controls
  category structure entirely from data_adder — no code change needed
  to add, rename, reorder, or remove categories.

================================================================
TASK 0 — DB SCHEMA & INFRASTRUCTURE FIXES
================================================================

priority: must be done before any UI task

0-A: Tool struct — add short_desc TEXT column
  files:
    L0-core/include/DatabaseManager.h   → add string short_desc to Tool struct
    L0-core/src/DatabaseManager.cpp     → update createTables() SQL
    data_adder.cpp                      → add prompt for short_desc

0-B: Template struct — add description TEXT column
  files:
    L0-core/include/DatabaseManager.h   → add string description to Template struct
    L0-core/src/DatabaseManager.cpp     → update createTables() SQL
    data_adder.cpp                      → add prompt for description

0-C: UserDataManager — JSON-based user data storage
  files:
    L0-core/include/path_resolver.h
      → add: userDataDir()        — base dir for user data (platform-aware)
      → add: savedCommandsFile()  — path to saved_commands.json
      → add: savedScriptsFile()   — path to saved_scripts.json
      → add: scriptsDir()         — path to generated scripts folder
      → add macOS support:
            macOS → ~/Library/Application Support/tguide/
      → add all new dirs to createDirs()

    L0-core/include/UserDataManager.h   (NEW FILE)
      → class UserDataManager:
          UserDataManager(const std::string& commandsPath,
                          const std::string& scriptsPath)
          bool load()
          bool save()
          int  saveCommand(int tool_id, const std::string& command,
                           const std::string& note)
          bool deleteCommand(int id)
          std::vector<SavedCommand> getCommands()
          int  saveScript(const std::string& name, const std::string& path,
                          const std::string& note)
          bool deleteScript(int id)
          std::vector<SavedScript> getScripts()
      → structs:
          SavedCommand { int id; int tool_id; std::string command; std::string note; }
          SavedScript  { int id; std::string name; std::string path; std::string note; }

    L0-core/src/UserDataManager.cpp     (NEW FILE)
      → implement UserDataManager using json.hpp
      → auto-create files with empty arrays if missing
      → id management: find max id in array + 1

    CoreRunner.cpp
      → construct UserDataManager after config load, before DBCache::init()
      → check files accessible, UI_fatal if not

0-D: config.json + config_manager — add new keys
  files:
    config/config.json                  → add "colors": 1 and "disclaimer_accepted": 0
                                           at root level (not nested under App)
    L0-core/src/config_manager.cpp      → add both keys to defaultConfig so they are
                                           auto-created on first run if missing
    L2-Interface_Engine/includes/UI_colors.h → add runtime function colorsEnabled()
                                               that reads config instead of compile-time
                                               #ifdef only — Windows still forced off

0-E: legal disclaimer check
  files:
    L0-core/src/config_manager.cpp      → add "disclaimer_accepted": 0 to defaultConfig
    CoreRunner.cpp                      → check disclaimer before UIEngine::start()
    L2-Interface_Engine/src/UI_disclaimer.cpp  → new file, show disclaimer screen
    L2-Interface_Engine/includes/UI_disclaimer.h

  flow:
    on startup → ConfigManager reads "disclaimer_accepted"
    if 0 → show disclaimer screen
    if user types "agree" → set "disclaimer_accepted": 1 via ConfigManager::save()
    if user types "exit"  → quit immediately
    if 1 → skip disclaimer, proceed normally

verify before moving on:
  - project compiles cleanly
  - data_adder handles new fields
  - saved_commands.json and saved_scripts.json created on first run
  - correct paths per platform (Linux, Termux, Windows, macOS)
  - config.json has colors key

================================================================
TASK 1 — navigation system (global input handler)
================================================================

goal: implement reusable input handling used by all screens

1-A: InputHandler — new utility
  files:
    L2-Interface_Engine/includes/UI_input.h
    L2-Interface_Engine/src/UI_input.cpp

  functions:
    string  readInput(const string& prompt)
    bool    isBack(const string& input)      → "0" or "back"
    bool    isQuit(const string& input)      → "q", "quit", "exit"
    bool    isNext(const string& input)      → "next" or "n"
    bool    isPrev(const string& input)      → "prev" or "p"
    int     toNumber(const string& input)    → -1 if not a number
    void    handleQuit()                     → clean exit with goodbye message

  rule: every screen checks isQuit() and isBack() before any other logic

1-B: Paginator — new utility
  files:
    L2-Interface_Engine/includes/UI_paginator.h
    L2-Interface_Engine/src/UI_paginator.cpp

  template class or struct that takes a vector<T> and renders pages of 10
  supports: next, prev, select by number, back
  clears screen on every page change
  shows [page X/Y] indicator


================================================================
TASK 1b — PROFESSIONAL ERROR HANDLING SYSTEM
================================================================

priority: implement before any UI screen (Tasks 2–9)
reason: every screen depends on consistent error behavior

goal:
  every error in the application is anticipated, categorized,
  and handled with a defined automatic recovery path or a clear
  user-facing message. nothing unexpected ever happens silently.

================================================================
ERROR CATEGORIES AND BEHAVIOR
================================================================

CATEGORY 1 — FATAL (UI_fatal)
  definition: the application cannot continue, no recovery possible
  behavior:
    - display error message clearly
    - wait for user to press enter (do NOT clear screen after)
    - return through call stack — never call exit() directly
    - screen is never cleared after a fatal message so the user
      can read it fully before the terminal closes
  examples:
    - system directories cannot be created (no root on Linux)
    - database not found, no download URL set
    - downloaded database hash mismatch (tampering detected)
    - database fatal initialization failure

CATEGORY 2 — RECOVERABLE ERROR (UI_errors)
  definition: a problem occurred but the application recovered
  behavior:
    - display error message
    - DO NOT clear the screen immediately after showing it
    - caller waits with pause() so user can read the message
    - application continues after user presses enter
    - automatic recovery is attempted before calling UI_errors
  examples:
    - user data directories failed to create (app continues without saved data)
    - JSON file malformed → auto-reset to empty, UI_errors explains this
    - single DB entry malformed → skipped, UI_errors notes it
    - config key missing → default applied, UI_errors if non-standard
    - file write failure → rolled back, UI_errors reports it

CATEGORY 3 — SILENT AUTO-RECOVERY
  definition: problem detected, fixed automatically, user not notified
  behavior: no message, no pause, application continues normally
  examples:
    - config.json missing → auto-created with defaults (expected on first run)
    - saved_commands.json missing → auto-created empty (expected on first run)
    - saved_scripts.json missing → auto-created empty (expected on first run)
    - .db_cache missing → auto-created (expected on first run)
    - user data dirs missing → auto-created (expected on first run)
    - JSON key absent → replaced with default silently

CATEGORY 4 — INPUT VALIDATION ERRORS
  definition: user provided invalid input, prompt is re-shown
  behavior:
    - print short inline message (no pause, no screen clear)
    - re-display the same prompt immediately
    - never crash or exit on bad user input
  examples:
    - invalid menu choice → "invalid choice — try again."
    - ambiguous prefix match → "ambiguous input — be more specific."
    - empty input → re-prompt silently
    - out-of-range number → "invalid choice — try again."
    - invalid filter expression → show usage hint + re-prompt
    - script filename contains / or \ → "invalid filename — try again."
    - script name already exists → "file exists. overwrite? (y/n)"

================================================================
RULE: ERROR MESSAGES MUST SURVIVE SCREEN CLEAR
================================================================

A common bug: show an error message then immediately call
clearScreen(), erasing it before the user can read it.

RULE: after any UI_errors() call, always call pause() before
the next clearScreen(). This ensures the user sees and
acknowledges the message.

correct pattern:
  UI_errors("something went wrong with the JSON file.");
  pause();
  // only now is it safe to clear screen or continue

incorrect pattern (BUG):
  UI_errors("something went wrong.");
  UI::clearScreen();   ← ERROR MESSAGE NEVER SEEN

UI_fatal() must NEVER be followed by clearScreen().
The terminal output is the only record of what went wrong.

================================================================
ANTICIPATED INPUT EDGE CASES (handle in UI_input / all screens)
================================================================

every screen must handle these without crashing:

  empty input          → re-prompt (no message for most screens)
  whitespace only      → treated as empty after normalize()
  very long input      → truncate to 256 chars before processing
  non-ASCII input      → normalize() strips control chars, keeps UTF-8
  numeric overflow     → toNumber() catches via try/catch → returns -1
  EOF on stdin         → readInput() returns "" → treated as quit
  ctrl+C / SIGINT      → not caught — OS handles, destructors run
  number "0"           → isBack() catches before toNumber() is called
  negative number      → toNumber() returns -1 (no digits-only match)
  decimal input "1.5"  → toNumber() returns -1 (non-digit char found)
  repeated spaces      → normalize() collapses to single result
  mixed case "BACK"    → normalize() lowercases → "back" → isBack() true
  tab as separator     → normalize() strips tabs
  option name with spaces → matchOption() handles first-word extraction

================================================================
ANTICIPATED FUNCTION RETURN EDGE CASES
================================================================

all callers must check these:

  DatabaseManager query returns empty vector
    → show "no results found" message, do not attempt to index
  DatabaseManager returns false on execute()
    → UI_errors(), operation rolled back, continue

  UserDataManager::saveCommand() returns -1
    → UI_errors("failed to save command."), do not assume save succeeded
  UserDataManager::deleteCommand() returns false
    → UI_errors("failed to delete command."), item still in list

  Paginator::select() returns -1
    → "invalid choice — try again." (input validation category)
  Paginator::nextPage() returns false
    → "already on last page." (inline, no pause, re-render)
  Paginator::prevPage() returns false
    → "already on first page." (inline, no pause, re-render)

  toNumber() returns -1
    → not a number — try matchOption() or show invalid input
  matchOption() returns -1
    → no match or ambiguous — show appropriate message

  readInput() returns ""
    → EOF reached — treat as quit, call handleQuit() and return

  config save fails
    → UI_errors("failed to save settings."), setting reverted in memory

  script file write fails
    → UI_errors("failed to write script file."), JSON record not saved
  script file read fails (print content)
    → UI_errors("script file not found at: <path>")

  JSON parse completely fails
    → auto-reset to empty structure, UI_errors("data file was corrupt
       and has been reset. previous entries are lost.")

================================================================
ANTICIPATED FILESYSTEM EDGE CASES
================================================================

  path does not exist    → create_directories() with error_code
  path exists as file    → detect with is_directory(), UI_errors
  no write permission    → create_directories() returns error_code
  disk full              → ofstream.good() check after write
  file deleted mid-run   → re-create on next load()
  symlink target missing → exists() returns false → re-create

================================================================
IMPLEMENTATION REQUIREMENTS
================================================================

UI_errorHandling.h/cpp — verify and update:
  UI_fatal(msg)
    - print message
    - print "press enter to exit..."
    - wait for enter
    - return (caller exits)
    - NEVER followed by clearScreen() at any call site

  UI_errors(msg)
    - print message
    - print "press enter to continue..."
    - wait for enter
    - return (application continues)
    - NEVER followed by clearScreen() without this pause first

  UI_attention(msg)
    - used for decisions requiring user input (y/n)
    - print message + prompt
    - return char (0 on EOF/failure)
    - caller handles the char

  pause()
    - waits for enter key
    - used standalone when no message is needed
    - safe on all platforms

files to review and update:
  L2-Interface_Engine/src/UI_errorHandling.cpp  (verify current impl)
  L2-Interface_Engine/includes/UI_errorHandling.h
  — add pause() after every UI_errors() call site found in other files
  — audit all clearScreen() call sites for preceding errors

================================================================
TASK ORDER POSITION
================================================================

  Execute after Task 1, before Task 2.
  All screen implementations (Tasks 2–9) depend on this being correct.
  Tasks that were already implemented (Task 0 infrastructure) should
  be audited against these rules during this task.

================================================================
TASK 2 — UI_tools: main entry + category browser
================================================================

goal: tools main screen → category list → tools in category

screen 1: tools entry
  tguide › tools
  ├─ ◉  Browse by Category   [1]
  ├─ ⌕  Search               [2]
  ├─ ⊟  Filter               [3]
  └─ ←  Back                 [0]

screen 2: category list
  tguide › tools › categories
  fetched via svc_tools::getCategories()
  displayed as paginated numbered list
  user picks number or types back/0

screen 3: tools in category
  tguide › tools › <category>
  each item shows:
    [N]  TOOL NAME          ← bold + cyan
         one-line short_desc ← dim
  paginated, 10 per page
  user picks number → goes to TASK 4 (tool detail)

files:
  L2-Interface_Engine/src/UI_tools.cpp
  L1-services/src/svc_tools.cpp
  L1-services/includes/svc_tools.h

svc_tools functions needed:
  vector<string>  getCategories()
  vector<Tool>    getToolsByCategory(const string& category)

================================================================
TASK 3 — UI_tools: search screen
================================================================

goal: search UI ready, input accepted, no algorithm yet

screen:
  tguide › tools › search
  enter search query (or 0 to go back):
  → <input>

after input:
  show "searching..." then empty results with message:
  "search engine not yet implemented — coming in a future update"
  results area is ready structurally for when algorithm is added

files:
  L2-Interface_Engine/src/UI_tools.cpp  (add search section)
  L1-services/src/svc_tools.cpp         (stub: searchTools → returns empty)
  L1-services/includes/svc_tools.h

TODO marker in svc_tools::searchTools() for future algorithm

================================================================
TASK 4 — tool detail screen (shared endpoint)
================================================================

goal: shown after user selects any tool from any path

screen:
  tguide › tools › <tool_name>
  ├─ ◉  Description              [A]
  ├─ ⊞  All Flags                [B]
  ├─ ⊟  Flags with Filter        [C]
  ├─ ▦  Templates                [D]
  ├─ ▦  Templates with Filter    [E]
  └─ ←  Back                     [0]

A: show full description from DB
   clear screen, printBreadcrumb, display description block, pause to read
   back returns to tool detail menu

B: all flags — paginated list
   each flag shows: name | description | protocols | root | loud

C: flags with filter — placeholder screen
   show available filters + usage hint
   accept filter input
   implementation pending (marked TODO)

D: templates — paginated list
   each template: name on one line, description below (dim)
   user picks → goes to TASK 5 (template detail)

E: templates with filter — same as C but for templates
   implementation pending (marked TODO)


  NOTE: Tool Detail screen checks tool name at render time:
    if tool name == "metasploit" → add Vulnerabilities option [F]
    if tool name == "recon-ng"   → add Modules option [F]
  The check is case-insensitive. All other tools show options A-E only.

files:
  L2-Interface_Engine/src/UI_tools.cpp
  L1-services/src/svc_tools.cpp

svc_tools functions needed:
  Tool              getToolById(int id)
  vector<ToolFlag>  getFlagsForTool(int tool_id)
  vector<Template>  getTemplatesForTool(int tool_id)


================================================================
TASK 4b — metasploit: Vulnerabilities sub-menu
================================================================

goal: dedicated vulnerabilities browser for metasploit tool

screen:
  tguide › tools › metasploit › vulnerabilities
  ├─ ⌕  Search               [1]
  ├─ ⊟  Filter               [2]
  ├─ ⊞  Show All             [3]
  └─ ←  Back                 [0]

search:
  accepts input, returns empty results, TODO marker for algorithm

filter:
  available filters (from vulnerabilities table):
    severity   — severity=critical / severity=high / severity=medium / severity=low
    service    — service=ftp / service=smb / service=http
    platform   — platform=linux / platform=windows
    access     — access=remote / access=local
  usage: severity=critical,service=ftp

show all:
  paginated list (10 per page), each entry shows:
    name | metasploit_name | severity | service | platform

  user picks entry → full vulnerability detail:
    name, metasploit_name, severity, access, platform, service,
    discovered_date, discoverer, description, danger
    + options list if any

files:
  L2-Interface_Engine/src/UI_tools.cpp  (extend tool detail section)
  L1-services/src/svc_tools.cpp

svc_tools functions needed:
  vector<Vulnerability>  getAllVulnerabilities()
  vector<Vulnerability>  getVulnerabilitiesByFilter(filters)
  vector<Option>         getOptionsForVuln(int vuln_id)

================================================================
TASK 4c — recon-ng: Modules sub-menu
================================================================

goal: dedicated modules browser for recon-ng tool

screen:
  tguide › tools › recon-ng › modules
  ├─ ⌕  Search               [1]
  ├─ ⊟  Filter               [2]
  ├─ ⊞  Show All             [3]
  └─ ←  Back                 [0]

search:
  accepts input, returns empty results, TODO marker for algorithm

filter:
  available filters (from modules table):
    type      — type=recon / type=discovery / type=exploitation
    platform  — platform=linux / platform=windows
    mode      — mode=active / mode=passive
    loud      — loud=yes / loud=no
  usage: type=recon,mode=passive

show all:
  paginated list (10 per page), each entry shows:
    name | type | mode | loud

  user picks entry → full module detail:
    name, path, platform, type, description, API, mode, loud, output

files:
  L2-Interface_Engine/src/UI_tools.cpp  (extend tool detail section)
  L1-services/src/svc_tools.cpp

svc_tools functions needed:
  vector<Module>  getAllModules()
  vector<Module>  getModulesByFilter(filters)

================================================================
TASK 5 — template detail + fill values + save
================================================================

goal: show template, fill placeholders, display ready command, save option

flow:
  user selected a template
  → show template name + description
  → detect placeholders in template flag string:
      <target_ip>, <target_domain>, <target_email>,
      <target_phone>, <port>, <wordlist>, <output_file>
  → prompt user to fill each one individually
  → display final built command in a clear box:
      ┌─────────────────────────────────────┐
      │  nmap -sV -p 80 192.168.1.1        │
      └─────────────────────────────────────┘
  → options:
      [S] Save to Saved Commands
      [0] Back without saving

if Save:
  prompt for a short note (one sentence)
  call svc_tools::saveCommand(tool_id, command, note)
  confirm saved → back to tool detail

files:
  L2-Interface_Engine/src/UI_tools.cpp
  L1-services/src/svc_tools.cpp
  L0-core/src/UserDataManager.cpp   (UserDataManager::saveCommand)

================================================================
TASK 6 — Saved Commands screen
================================================================

goal: view all saved commands from saved_commands.json

screen:
  tguide › saved commands
  paginated list:
    [N]  tool_name — note
         command string (dim)

user picks one:
  options:
    [V] View full command (copyable block)
    [D] Delete
    [0] Back

files:
  L2-Interface_Engine/src/UI_savedCommands.cpp
  L1-services/src/svc_savedCommands.cpp
  L1-services/includes/svc_savedCommands.h

================================================================
TASK 7 — Script Generator
================================================================

goal: pick multiple templates, fill values, generate .sh file

flow:
  screen 1: pick a tool → pick templates (multi-select)
    user keeps adding templates or types "done" when finished
    each template: fill placeholders same as TASK 5

  screen 2: review all commands before generating
    numbered list of all commands to be written
    user can remove one by number or confirm

  screen 3: name the script file
    enter filename (without extension) → saved as <name>.sh
    path: ~/.local/share/tguide/scripts/<name>.sh

  screen 4: confirm
    show full script path
    options:
      [G] Generate and save
      [0] Cancel

generated .sh file format:
  #!/bin/bash
  # generated by tguide — voidoxin
  # <script_name>
  <command_1>
  <command_2>
  ...

"run from inside tguide" feature → future task (marked TODO)

files:
  L2-Interface_Engine/src/UI_generator.cpp
  L1-services/src/svc_generator.cpp
  L1-services/includes/svc_generator.h

================================================================
TASK 8 — Saved Scripts screen
================================================================

goal: view saved scripts list, open path, delete

screen:
  tguide › saved scripts
  paginated list:
    [N]  script_name
         path (dim)
         note (dim)

user picks one:
  options:
    [P] Print script content
    [D] Delete record (does not delete file)
    [0] Back

files:
  L2-Interface_Engine/src/UI_savedScripts.cpp
  L1-services/src/svc_savedScripts.cpp
  L1-services/includes/svc_savedScripts.h

================================================================
TASK 9 — Settings screen
================================================================

goal: toggle color output, reflect change immediately

screen:
  tguide › settings
  ├─ ◉  Colors   [enabled / disabled]   [1]
  └─ ←  Back                            [0]

toggle colors:
  reads current value from config.json "colors" key
  toggles 1 → 0 or 0 → 1
  saves via ConfigManager
  applies immediately to current session via UI_colors runtime check

files:
  L2-Interface_Engine/src/UI_settings.cpp
  UI_colors.h   → add runtime function: bool colorsEnabled()
  config_manager.cpp → "colors": 1 in defaults

================================================================
TASK ORDER (recommended execution sequence)
================================================================

  ── FOUNDATION (must be complete before any UI work) ──────────────
  TASK 0   → DB schema + categories table + UserDataManager + disclaimer + config
  TASK 1   → navigation system + paginator
  TASK 1b  → error handling contract

  ── CORE FEATURE: TOOLS (primary value of the tool) ───────────────
  TASK 2   → tools entry + category browser
  TASK 3   → search screen (UI stub)
  TASK 4   → tool detail screen (flags, templates)
  TASK 4b  → metasploit: vulnerabilities sub-menu
  TASK 4c  → recon-ng: modules sub-menu
  TASK 5   → template fill + placeholder prompting + save command

  ── USER DATA (depends on TASK 5 for save flow) ───────────────────
  TASK 6   → saved commands screen
  TASK 8   → saved scripts screen

  ── SCRIPT GENERATOR (depends on TASK 5 template fill) ───────────
  TASK 7   → script generator (multi-template → .sh file)

  ── SETTINGS & POLISH ─────────────────────────────────────────────
  TASK 9   → settings screen (color toggle)

  ── FUTURE (after all screens complete) ───────────────────────────
  FUTURE-1 → search algorithm (inverted index + scoring + fuzzy)
  FUTURE-2 → flags filter (tool detail option C)
  FUTURE-3 → templates filter (tool detail option E)
  FUTURE-4 → vulnerability filter
  FUTURE-5 → module filter
  FUTURE-6 → run script from inside tguide
  FUTURE-7 → script generator pre-loaded from tool detail
  FUTURE-8 → apt/deb packaging + GitHub releases

================================================================
PENDING / FUTURE TASKS (not in current scope)
================================================================

  FUTURE-1: search algorithm — see FUTURE-1 section above (promoted)
  FUTURE-2: flags filter implementation (TASK 4 option C)
  FUTURE-3: templates filter implementation (TASK 4 option E)
  FUTURE-4: run script from inside tguide (TASK 7 extension)
  FUTURE-5: Script Generator accessible from tool detail screen
            (pre-load selected tool)


================================================================
FUTURE-1 (PROMOTED): Search Algorithm — svc_tools
================================================================

priority: implement after Task 9
reason: search UI stub already exists — algorithm slots in without
        touching any other file

PIPELINE:

  1. normalize(query) → lowercase, strip control chars, trim
     tokenize → split on whitespace → vector<string> tokens

  2. InvertedIndex — built once per session on first search call
       unordered_map<string, vector<int>>
       key   = normalized word
       value = list of tool IDs
       indexed fields: name, short_desc, description

  3. Candidate retrieval
       union of all token matches from index → candidate tool IDs

  4. Relevance scoring per candidate:

       +100  exact match on tool name
       +70   tool name contains a token
       +50   short_desc contains a token
       +30   description contains a token
       +20 × (matched_tokens / total_tokens)  ← coverage bonus
       -10 × levenshtein_distance             ← fuzzy penalty

  5. Fuzzy fallback — only if candidates < 3
       Levenshtein distance ≤ 2 against index keys
       only applied to tokens of length ≥ 4 (avoids noise on short tokens)
       standard DP, O(n×m), capped at distance 3

  6. Sort descending by score → paginated results

IMPLEMENTATION:

  files:
    L1-services/src/svc_tools.cpp   — buildIndex(), searchTools()
    L1-services/includes/svc_tools.h — searchTools() declaration

  static index struct (file-scope in svc_tools.cpp):
    struct SearchIndex {
        unordered_map<string, vector<int>> index;
        vector<Tool>                       tools;
        bool                               ready = false;
    };
    static SearchIndex s_index;

  searchTools(ToolD& db, const string& query) → vector<Tool>
    - builds index on first call, cached for the session
    - returns results sorted by score, max 50 results
    - returns empty vector if query is empty after normalize()

================================================================
FILES TO BE CREATED OR MODIFIED (full list)
================================================================

MODIFIED:
  L0-core/include/DatabaseManager.h
  L0-core/src/DatabaseManager.cpp
  L0-core/include/path_resolver.h
  L0-core/src/config_manager.cpp
  CoreRunner.cpp
  CMakeLists.txt
  data_adder.cpp
  L2-Interface_Engine/includes/UI_colors.h
  L2-Interface_Engine/src/UI_tools.cpp
  L2-Interface_Engine/src/UI_generator.cpp
  L2-Interface_Engine/src/UI_savedCommands.cpp
  L2-Interface_Engine/src/UI_savedScripts.cpp
  L2-Interface_Engine/src/UI_settings.cpp
  L1-services/src/svc_tools.cpp
  L1-services/src/svc_generator.cpp
  L1-services/src/svc_savedCommands.cpp
  L1-services/src/svc_savedScripts.cpp
  L1-services/src/svc_settings.cpp

CREATED:
  L2-Interface_Engine/includes/UI_input.h
  L2-Interface_Engine/src/UI_input.cpp
  L2-Interface_Engine/includes/UI_paginator.h
  L2-Interface_Engine/src/UI_paginator.cpp
  L2-Interface_Engine/includes/UI_disclaimer.h
  L2-Interface_Engine/src/UI_disclaimer.cpp
