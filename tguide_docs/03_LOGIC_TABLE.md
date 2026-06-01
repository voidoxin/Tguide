# tguide — Logic Table, All States & Development Notes
**Author:** voidoxin  
**Read after:** `01_PROJECT_OVERVIEW.md` and `02_USER_FLOW.md`

---

## 1. Startup State Machine

| State | Condition | Action | Next State |
|-------|-----------|--------|-----------|
| `BOOT` | Always | Check write access | `CHECK_ACCESS` |
| `CHECK_ACCESS` | Linux + not root | `UI_fatal` + exit | `TERMINATED` |
| `CHECK_ACCESS` | Termux / Windows / macOS | Pass | `CREATE_DIRS` |
| `CREATE_DIRS` | System dirs fail | `UI_fatal` + exit | `TERMINATED` |
| `CREATE_DIRS` | User dirs fail | `UI_errors` + continue | `LOAD_CONFIG` |
| `CREATE_DIRS` | All pass | Continue | `LOAD_CONFIG` |
| `LOAD_CONFIG` | config.json missing | Auto-create with defaults | `INIT_COLORS` |
| `LOAD_CONFIG` | config.json malformed | Reset to defaults | `INIT_COLORS` |
| `LOAD_CONFIG` | config.json valid | Load | `INIT_COLORS` |
| `INIT_COLORS` | Windows / macOS | Force colors off | `LOAD_USER_DATA` |
| `INIT_COLORS` | Linux / Termux | Read `config["colors"]` | `LOAD_USER_DATA` |
| `LOAD_USER_DATA` | JSON files missing | Auto-create empty | `DISCLAIMER` |
| `LOAD_USER_DATA` | JSON files malformed | Skip malformed entries, continue | `DISCLAIMER` |
| `LOAD_USER_DATA` | JSON files valid | Load | `DISCLAIMER` |
| `DISCLAIMER` | `disclaimer_accepted == 1` | Skip | `INIT_DB_CACHE` |
| `DISCLAIMER` | `disclaimer_accepted == 0` | Show disclaimer screen | `DISCLAIMER_INPUT` |
| `DISCLAIMER_INPUT` | User types `agree` | Save acceptance, clear screen | `INIT_DB_CACHE` |
| `DISCLAIMER_INPUT` | User types `exit/q/quit` | Clean return, exit | `TERMINATED` |
| `DISCLAIMER_INPUT` | Any other input | Redisplay disclaimer | `DISCLAIMER_INPUT` |
| `DISCLAIMER_INPUT` | EOF (stdin closed) | Clean return, exit | `TERMINATED` |
| `INIT_DB_CACHE` | Cache file missing | Auto-create | `INIT_DB` |
| `INIT_DB_CACHE` | Cache file exists | Load | `INIT_DB` |
| `INIT_DB` | DB valid + hash matches | Use DB | `UI_READY` |
| `INIT_DB` | DB valid + hash differs + has data | Prompt user | `DB_CONFLICT` |
| `INIT_DB` | DB at default path | Move to config path | `UI_READY` |
| `INIT_DB` | No local DB | Download from GitHub | `DOWNLOAD` |
| `INIT_DB` | Fatal failure | `UI_fatal` + exit | `TERMINATED` |
| `DB_CONFLICT` | User picks `n` (keep external) | Use external DB | `UI_READY` |
| `DB_CONFLICT` | User picks `y` (revert) | Copy official DB | `UI_READY` |
| `DOWNLOAD` | HTTPS URL set + success + hash matches | Use downloaded DB | `UI_READY` |
| `DOWNLOAD` | HTTPS URL set + hash mismatch | Delete file + `UI_fatal` | `TERMINATED` |
| `DOWNLOAD` | URL not set | `UI_fatal` | `TERMINATED` |
| `DOWNLOAD` | Download fails (timeout/network) | `UI_fatal` | `TERMINATED` |
| `UI_READY` | DB classes initialized | Hand off to `UIEngine::start()` | `MAIN_MENU` |

---

## 2. Main Menu State

| Input | Action |
|-------|--------|
| `1` or `tools` | → Tools screen |
| `2` or `script generator` | → Script Generator screen |
| `3` or `saved commands` | → Saved Commands screen |
| `4` or `saved scripts` | → Saved Scripts screen |
| `5` or `settings` | → Settings screen |
| `6` or `exit` / `q` / `quit` | Clean exit |
| `0` or `back` | Clean exit (main menu has no parent) |
| Any other | Redisplay with "invalid choice" |

---

## 3. Tools Screen — All States

### 3.1 Tools Entry

| Input | Action |
|-------|--------|
| `1` or `category` | → Category browser |
| `2` or `search` | → Search screen |
| `3` or `filter` | → Filter screen |
| `0` or `back` | → Main menu |
| `q/quit/exit` | Clean exit |

### 3.2 Category Browser

| State | Condition | Action |
|-------|-----------|--------|
| DB has categories | Normal | Display paginated category list |
| DB empty | No tools | Show "no categories found" + back |
| User types number | Valid index | Load tool list for that category |
| User types category name | Matches (case-insensitive) | Load tool list |
| User types number | Out of range | "invalid choice" |
| `next` / `n` | More pages exist | Next page |
| `next` / `n` | Last page | "already on last page" |
| `prev` / `p` | Not first page | Previous page |
| `prev` / `p` | First page | "already on first page" |
| `0` or `back` | Anywhere | → Tools entry |

### 3.3 Tool List (inside category)

| State | Condition | Action |
|-------|-----------|--------|
| Category has tools | Normal | Display paginated tool list with short_desc |
| Category empty | No tools | Show "no tools in this category" + back |
| User picks tool | Valid | → Tool Detail screen |
| User picks number | Out of range | "invalid choice" |
| User types tool name | Matches | → Tool Detail screen |
| `0` or `back` | Anywhere | → Category browser |

### 3.4 Search Screen

| State | Condition | Action |
|-------|-----------|--------|
| User enters query | Any input | Show "searching..." → empty results |
| Algorithm | Not implemented | Show TODO message |
| `0` or `back` | Anywhere | → Tools entry |

### 3.5 Filter Screen

| State | Condition | Action |
|-------|-----------|--------|
| User enters valid filter | Matches pattern | Query DB and show results |
| User enters invalid filter | Bad format | Show usage hint, retry |
| Results exist | Any | Display paginated tool list |
| Results empty | No matches | "no tools match the given filters" |
| `0` or `back` | Anywhere | → Tools entry |

### 3.6 Tool Detail Screen

| Input | Action |
|-------|--------|
| `A` or `description` | Show full description |
| `B` or `flags` | Show all flags (paginated) |
| `C` or `flags filter` | Show flags filter screen |
| `D` or `templates` | Show templates (paginated) |
| `E` or `templates filter` | Show templates filter screen |
| `F` or `vulnerabilities` | (metasploit only) → Vulnerabilities sub-menu |
| `F` or `modules` | (recon-ng only) → Modules sub-menu |
| `0` or `back` | → Tool list |

### 3.7 Template Detail

| State | Condition | Action |
|-------|-----------|--------|
| Template has placeholders | `<target_ip>` etc. detected | Prompt for each value |
| No placeholders | Direct template | Show command immediately |
| All values filled | Normal | Show copyable command block |
| User types `S` or `save` | Confirmed | Prompt for note → save to JSON |
| Save succeeds | JSON write OK | "command saved" confirmation |
| Save fails | JSON write error | `UI_errors` + continue without saving |
| `0` or `back` | Anywhere | → Template list |

---

## 4. Metasploit — Vulnerabilities Sub-Menu States

| Input | Action |
|-------|--------|
| `1` or `search` | → Vulnerability search screen |
| `2` or `filter` | → Vulnerability filter screen |
| `3` or `show all` or `all` | → Vulnerability list (paginated) |
| `0` or `back` | → Tool Detail screen |

### Search

| State | Condition | Action |
|-------|-----------|--------|
| User enters query | Any | Show TODO message, empty results |
| Algorithm | Not implemented | Placeholder — future task |

### Filter

Available filter fields from `vulnerabilities` table:

| Filter | Field | Example |
|--------|-------|---------|
| `severity` | severity TEXT | `severity=critical` |
| `service` | service TEXT | `service=ftp` |
| `platform` | platform TEXT | `platform=linux` |
| `access` | access TEXT | `access=remote` |

| State | Condition | Action |
|-------|-----------|--------|
| Valid filter expression | Matches DB records | Paginated vulnerability list |
| No matches | Zero results | "no vulnerabilities match the given filters" |
| Invalid format | Parse error | Show usage hint, retry |

### List All

| State | Condition | Action |
|-------|-----------|--------|
| DB has vulnerabilities | Normal | Paginated list (10 per page) |
| DB empty | No vulnerabilities | "no vulnerabilities in database" |
| User picks entry | Valid | Show vulnerability detail |

### Vulnerability Detail

Each vulnerability shows:
- Name + metasploit_name
- Severity / access / platform / service
- Discovered date + discoverer
- Description
- Danger level
- Options (if any)

| Input | Action |
|-------|--------|
| `0` or `back` | → Vulnerability list |

---

## 5. Recon-ng — Modules Sub-Menu States

| Input | Action |
|-------|--------|
| `1` or `search` | → Module search screen |
| `2` or `filter` | → Module filter screen |
| `3` or `show all` or `all` | → Module list (paginated) |
| `0` or `back` | → Tool Detail screen |

### Filter

Available filter fields from `modules` table:

| Filter | Field | Example |
|--------|-------|---------|
| `type` | type TEXT | `type=recon` |
| `platform` | platform TEXT | `platform=linux` |
| `mode` | mode INTEGER | `mode=active` or `mode=passive` |
| `loud` | loud INTEGER | `loud=yes` or `loud=no` |

### Module Detail

Each module shows:
- Name + path
- Platform / type
- Mode (active/passive) + loud (yes/no)
- Description
- API (if applicable)
- Output

| Input | Action |
|-------|--------|
| `0` or `back` | → Module list |

---

## 6. Script Generator — All States

| Stage | Input | Action |
|-------|-------|--------|
| Pick tool | Tool name or number | Load tool's templates |
| Pick tool | Unknown name | "tool not found, try again" |
| Pick template | Number | Fill placeholders |
| Pick template | `done` | → Review stage |
| Pick template | `0` or `back` | Discard all, → main menu |
| Fill placeholder | Any input | Stored as value |
| Fill placeholder | Empty | Re-prompt |
| Review | Number | Remove that command |
| Review | `confirm` | → Name stage |
| Review | `0` or `back` | Discard all, → main menu |
| Name script | Valid filename | → Confirm stage |
| Name script | Contains `/` or `\` or `.` | "invalid filename, try again" |
| Name script | Already exists | "file exists, overwrite? y/n" |
| Confirm | `G` or `generate` | Write `.sh` file, save record to JSON |
| Confirm | `0` or `back` | Discard all, → main menu |
| Generate | Write succeeds | Confirmation + path display |
| Generate | Write fails | `UI_errors` + return to main menu |

---

## 7. Saved Commands — All States

| State | Condition | Action |
|-------|-----------|--------|
| JSON file loads | Normal | Display paginated command list |
| JSON file empty | No saved commands | "no saved commands yet" |
| User picks entry | Valid | Show command detail |
| View full command | `V` | Display copyable block |
| Delete | `D` | Confirm → delete from JSON |
| Delete confirm | `y` | Remove entry, save JSON |
| Delete confirm | `n` | Cancel, return to detail |
| `0` or `back` | Anywhere | → Saved Commands list or main menu |

---

## 8. Saved Scripts — All States

| State | Condition | Action |
|-------|-----------|--------|
| JSON file loads | Normal | Display paginated script list |
| JSON file empty | No saved scripts | "no saved scripts yet" |
| User picks entry | Valid | Show script detail |
| Print content | `P` | Read `.sh` file and display |
| Print content | File not found on disk | "script file not found at path" |
| Delete record | `D` | Confirm → remove from JSON (file not deleted) |
| `0` or `back` | Anywhere | → Saved Scripts list or main menu |

---

## 9. Settings — All States

| Input | Current State | Action |
|-------|--------------|--------|
| `1` or `colors` | Colors enabled | Disable, save config, apply immediately |
| `1` or `colors` | Colors disabled | Enable, save config, apply immediately |
| `0` or `back` | Any | → Main menu |
| Config save fails | On toggle | `UI_errors` + revert toggle value |

---

## 10. Error Handling — All Cases

| Error Type | Where | Behavior |
|-----------|-------|---------|
| Fatal — root required | Startup | `UI_fatal` + `return 1` in main |
| Fatal — system dirs fail | Startup | `UI_fatal` + `return 1` in main |
| Fatal — DB download tampered | Startup | `UI_fatal` + `return 1` in main |
| Fatal — DB not found, no URL | Startup | `UI_fatal` + `return 1` in main |
| Fatal — DB download fails | Startup | `UI_fatal` + `return 1` in main |
| Non-fatal — user dirs fail | Startup | `UI_errors` + continue |
| Non-fatal — JSON load fails | Startup | `UI_errors` + continue with empty data |
| Non-fatal — JSON write fails | Any save | `UI_errors` + operation rolled back |
| Non-fatal — DB open error | Any query | `UI_errors` + return empty result |
| Non-fatal — SQLite prepare fail | Any query | Return false / empty |
| Non-fatal — backup fails | Any write | `UI_errors` + continue |
| `exit()` direct call | **NEVER** | Always `return` through call stack |

---

## 11. Placeholder Reference

Templates can contain these placeholders. The user is prompted for each one:

| Placeholder | Description | Example input |
|-------------|-------------|---------------|
| `<target_ip>` | Target IP address | `192.168.1.1` |
| `<target_domain>` | Target domain name | `example.com` |
| `<target_email>` | Target email address | `user@example.com` |
| `<target_phone>` | Target phone number | `+1-555-0100` |
| `<port>` | Port number or range | `80` or `1-1000` |
| `<wordlist>` | Path to wordlist file | `/usr/share/wordlists/rockyou.txt` |
| `<output_file>` | Output file path | `results.txt` |
| `<subnet>` | Subnet mask | `24` |

---

## 12. Pending / Future Tasks

| ID | Description | Status |
|----|-------------|--------|
| FUTURE-1 | Search algorithm in `svc_tools::searchTools()` | Not started |
| FUTURE-2 | Flags filter implementation (Tool Detail option C) | Placeholder exists |
| FUTURE-3 | Templates filter implementation (Tool Detail option E) | Placeholder exists |
| FUTURE-4 | Vulnerability filter implementation | Placeholder exists |
| FUTURE-5 | Module filter implementation | Placeholder exists |
| FUTURE-6 | Run script from inside tguide (Script Generator extension) | Not started |
| FUTURE-7 | Script Generator pre-loaded from Tool Detail screen | Not started |

---

## 13. All Developer Notes (Consolidated)

The following notes were recorded throughout the development process and must be resolved before release:

### Database Notes

- `tools.category` — TEXT column in `tools` table. Categories derived via `SELECT DISTINCT`. Not a separate table.
- `tools.short_desc` — Added in Task 0. One sentence shown under tool name in list views.
- `tools.description` — Full multi-line text shown in detail view.
- `templates.description` — Added in Task 0. Explains what the template accomplishes.
- `DB_OFFICIAL_HASH` — Empty during development. **Must be set before first public release.**
- `DB_DOWNLOAD_URL` — Empty during development. **Must be set before first public release.**

### Dev-Only Code (Remove Before Release)

| Code | Location | Reason |
|------|----------|--------|
| `BackupManager` | `DatabaseManager.h/cpp` + `CoreRunner.cpp` | Dev-only write protection |
| `add()` / `del()` methods | All DB classes | Only used by `data_adder.cpp` |
| `data_adder.cpp` | Project root | DB population tool, not for users |

### Code Quality Notes

- `using namespace std;` formatting issue in `CoreRunner.cpp` line 19 — leading spaces. Not fixed yet (out of scope for current tasks).
- `DB_OFFICIAL_HASH` and `DB_DOWNLOAD_URL` must be set and tested as a release milestone.

### Architecture Decisions (Locked)

- **No SQLite for user data.** JSON chosen for simplicity and human readability.
- **`exit()` is forbidden** everywhere in the codebase. Always return through the call stack.
- **`UI::clearScreen()`** from `UI_utils` is the only valid way to clear the screen. Never redefine it locally.
- **Constructor auto-load is removed** from `UserDataManager`. Caller must call `load()` explicitly.
- **`createDirs()` is split** into `createSystemDirs()` (fatal) and `createUserDirs()` (non-fatal).
- **HTTPS enforced** on all download URLs. Any `http://` URL is rejected before curl runs.

---

## 14. Full Development Task Plan

### Task 0 — Infrastructure (COMPLETE)
- [x] `tools.short_desc` column added
- [x] `templates.description` already existed — no changes needed
- [x] `UserDataManager` — JSON-based, `saved_commands.json` + `saved_scripts.json`
- [x] `PathResolver` — `userDataDir()`, `savedCommandsFile()`, `savedScriptsFile()`, `scriptsDir()`, `cacheFile()`, macOS support
- [x] `config.json` — `"colors": 1`, `"disclaimer_accepted": 0` added
- [x] `ConfigManager` — defaults updated
- [x] `UI_colors.h` — `initColors()`, `colorsEnabled()` runtime toggle
- [x] `UIDisclaimer` — first-run legal screen, returns `bool`
- [x] `CoreRunner` — full bootstrap sequence with correct error handling

### Task 1 — Navigation System
- [ ] `UI_input.h/cpp` — `readInput`, `isBack`, `isQuit`, `isNext`, `isPrev`, `toNumber`, `handleQuit`
- [ ] `UI_paginator.h/cpp` — template paginator, 10 items/page, next/prev/select/back

### Task 2 — Tools Entry + Category Browser
- [ ] Tools entry screen (3 options)
- [ ] Category list (paginated)
- [ ] Tool list per category (paginated, name + short_desc)
- [ ] `svc_tools::getCategories()`, `svc_tools::getToolsByCategory()`

### Task 3 — Search Screen
- [ ] Search UI (accepts input, returns empty, TODO marker)
- [ ] `svc_tools::searchTools()` stub

### Task 4 — Tool Detail Screen
- [ ] Options A–E (+ F for metasploit, F for recon-ng)
- [ ] `svc_tools::getToolById()`, `getFlagsForTool()`, `getTemplatesForTool()`
- [ ] Vulnerabilities sub-menu (metasploit)
- [ ] Modules sub-menu (recon-ng)

### Task 5 — Template Fill + Save
- [ ] Placeholder detection and prompting
- [ ] Copyable command block display
- [ ] Save to `saved_commands.json` via `UserDataManager`

### Task 6 — Saved Commands Screen
- [ ] Paginated list
- [ ] View / Delete options
- [ ] `svc_savedCommands` bridge

### Task 7 — Script Generator
- [ ] Multi-tool, multi-template selection
- [ ] Placeholder fill per template
- [ ] Review + remove commands
- [ ] `.sh` file generation and save
- [ ] Record saved to `saved_scripts.json`

### Task 8 — Saved Scripts Screen
- [ ] Paginated list
- [ ] Print content / Delete record
- [ ] `svc_savedScripts` bridge

### Task 9 — Settings Screen
- [ ] Color toggle (read/write `config["colors"]`)
- [ ] Immediate session apply

### Future Tasks
- [ ] FUTURE-1: Search algorithm
- [ ] FUTURE-2: Flags filter (Tool Detail C)
- [ ] FUTURE-3: Templates filter (Tool Detail E)
- [ ] FUTURE-4: Vulnerability filter
- [ ] FUTURE-5: Module filter
- [ ] FUTURE-6: Run script from inside tguide
- [ ] FUTURE-7: Script Generator pre-loaded from Tool Detail

---

## 15. Error Handling — Complete Decision Table

### Fatal Errors (program cannot continue)

| Situation | Auto-recovery? | Action |
|-----------|---------------|--------|
| Linux + not root at startup | No | `UI_fatal` → `return 1` |
| System dirs creation fails | No | `UI_fatal` → `return 1` |
| DB not found + no download URL | No | `UI_fatal` → `return 1` |
| DB download fails (timeout/network) | No | `UI_fatal` → `return 1` |
| Downloaded DB hash mismatch | No | Delete file + `UI_fatal` → `return 1` |
| DB fatal init failure | No | `UI_fatal` → `return 1` |

**Rule:** after `UI_fatal()`, never call `clearScreen()`. Return immediately.

---

### Recoverable Errors (auto-recovery attempted, user notified)

| Situation | Auto-recovery | User message via |
|-----------|--------------|-----------------|
| User data dirs fail to create | App continues without saved data | `UI_errors` + `pause()` |
| saved_commands.json malformed | Reset to empty `{ "commands": [] }` | `UI_errors` + `pause()` |
| saved_scripts.json malformed | Reset to empty `{ "scripts": [] }` | `UI_errors` + `pause()` |
| Single JSON entry malformed | Entry skipped | `UI_errors` (no pause needed per entry) |
| JSON file write fails | Operation rolled back | `UI_errors` + `pause()` |
| DB entry read fails | Entry skipped | `UI_errors` |
| Config save fails | Setting reverted in memory | `UI_errors` + `pause()` |
| Script file write fails | JSON record not created | `UI_errors` + `pause()` |
| Script file missing on print | Show message in place of content | `UI_errors` + `pause()` |
| Backup fails | Backup skipped | `UI_errors` |

**Rule:** `UI_errors()` always followed by `pause()` before any `clearScreen()`.

---

### Silent Auto-Recovery (no user message)

| Situation | Auto-recovery |
|-----------|--------------|
| config.json missing | Auto-created with all defaults |
| saved_commands.json missing | Auto-created as `{ "commands": [] }` |
| saved_scripts.json missing | Auto-created as `{ "scripts": [] }` |
| .db_cache missing | Auto-created with default structure |
| User data dirs missing | Auto-created via `createUserDirs()` |
| Config key missing | Default value applied via merge/clean |

---

### Input Validation (inline message, re-prompt, no exit)

| Bad input | Response |
|-----------|---------|
| Empty input | Re-prompt silently |
| Whitespace only | Treated as empty after `normalize()` |
| Invalid menu number | `"  invalid choice — try again."` |
| Out-of-range number | `"  invalid choice — try again."` |
| Ambiguous prefix | `"  ambiguous — be more specific."` |
| No match at all | `"  invalid choice — try again."` |
| Invalid filter format | Show usage hint + re-prompt |
| Script name with `/` or `\` | `"  invalid filename — try again."` |
| Script name already exists | `"  file exists. overwrite? (y/n)"` |
| Decimal input `"1.5"` | `toNumber()` → -1 → invalid choice |
| Negative input `"-1"` | `toNumber()` → -1 → invalid choice |
| Numeric overflow `"99999999999"` | `toNumber()` catches → -1 → invalid |
| Mixed case `"BACK"` | `normalize()` → `"back"` → `isBack()` true |
| EOF on stdin | `readInput()` → `""` → treated as quit |
| Input longer than 256 chars | Truncated before processing |

---

## 16. Screen Clear Safety Rules

### The Error-Erase Bug

A common bug where error messages are erased immediately:
```
UI_errors("JSON file was corrupt.");
UI::clearScreen();   ← ERROR NEVER SEEN BY USER
```

### Correct Pattern

```
UI_errors("JSON file was corrupt and has been reset.");
pause();             ← user reads and presses enter
UI::clearScreen();   ← now safe to clear
```

### Rules by Error Type

| Type | Clear screen after? | Pause after? |
|------|---------------------|-------------|
| `UI_fatal()` | **NEVER** | Yes (built into UI_fatal) |
| `UI_errors()` | Only after `pause()` | **ALWAYS** |
| Input validation | No clear needed | No pause needed |
| Silent recovery | N/A | N/A |

