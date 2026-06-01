# tguide — Project Overview & Architecture
**Author:** voidoxin  
**Version:** V1.0.1 (in development)  
**Language:** C++17  
**Build System:** CMake 3.16+

---

## 1. What is tguide?

tguide is a terminal-based CLI reference tool designed for penetration testers and security researchers. It provides an offline, structured, and searchable database of security tools, their flags, usage templates, and known vulnerabilities — all accessible from a clean, color-coded terminal interface.

The goal is to eliminate the need to memorize command syntax or search online during an engagement. Everything a practitioner needs — flags, protocols, templates, exploits — is one or two keystrokes away.

**Target platforms:** Linux (primary), Termux/Android, Windows, macOS  
**Target audience:** Penetration testers, CTF players, security students

---

## 2. Core Design Principles

- **Offline-first.** The database ships with the binary. No internet required at runtime.
- **Zero runtime dependencies.** sqlite3 and json.hpp are bundled. libcurl is the only external dependency (used only for first-run DB download fallback).
- **Layered architecture.** Each layer has a single responsibility. UI never talks directly to the database.
- **Cross-platform.** Paths, colors, and permissions are resolved at runtime per platform.
- **Legal compliance.** First-run disclaimer is mandatory before any feature is accessible.

---

## 3. Project Structure

```
tguide/
├── CoreRunner.cpp               — entry point, bootstrap sequence
├── CMakeLists.txt               — build configuration
├── config/
│   └── config.json              — runtime configuration
├── data/
│   └── database/
│       └── tguide.db            — official read-only SQLite database
├── libs/
│   ├── sqlite3.c / sqlite3.h    — bundled SQLite (compiled as C object)
│   └── json.hpp                 — bundled nlohmann/json (header-only)
├── L0-core/                     — foundation layer
│   ├── include/
│   │   ├── DatabaseManager.h    — all DB structs and class declarations
│   │   ├── UserDataManager.h    — user data JSON structs and class
│   │   ├── config_manager.h     — config read/write interface
│   │   ├── db_cache_manager.h   — DB integrity cache
│   │   ├── path_resolver.h      — cross-platform path resolution
│   │   └── sha256.h             — SHA-256 implementation
│   └── src/
│       ├── DatabaseManager.cpp  — DB classes: VulnD, ModuD, ToolD, etc.
│       ├── UserDataManager.cpp  — JSON read/write for saved user data
│       ├── config_manager.cpp   — config merge/clean/save logic
│       ├── db_cache_manager.cpp — hash-based DB integrity tracking
│       └── sha256.cpp           — SHA-256 block processing
├── L1-services/                 — business logic layer (stubs, pending)
│   ├── includes/
│   │   ├── svc_tools.h
│   │   ├── svc_generator.h
│   │   ├── svc_savedCommands.h
│   │   ├── svc_savedScripts.h
│   │   └── svc_settings.h
│   └── src/  (implementations pending)
├── L2-Interface_Engine/         — UI layer
│   ├── includes/
│   │   ├── UI_Engine.h          — main menu loop
│   │   ├── UI_colors.h          — ANSI color constants + runtime toggle
│   │   ├── UI_utils.h           — clearScreen, banner, divider, breadcrumb
│   │   ├── UI_errorHandling.h   — UI_fatal, UI_errors, UI_attention, pause
│   │   ├── UI_disclaimer.h      — first-run legal screen
│   │   ├── UI_input.h           — global input handler (pending Task 1)
│   │   ├── UI_paginator.h       — paginated list renderer (pending Task 1)
│   │   ├── UI_tools.h           — tools browser screen
│   │   ├── UI_generator.h       — script generator screen
│   │   ├── UI_savedCommands.h   — saved commands screen
│   │   ├── UI_savedScripts.h    — saved scripts screen
│   │   └── UI_settings.h        — settings screen
│   └── src/  (implementations)
└── data_adder.cpp               — dev-only tool to populate tguide.db
```

---

## 4. Layer Architecture

```
┌─────────────────────────────────────────────────┐
│              CoreRunner.cpp                     │  Entry point
│         (bootstrap + init sequence)             │
└──────────────────────┬──────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────┐
│           L2-Interface_Engine                   │  UI Layer
│   Handles all terminal I/O, menus, colors       │
│   Never queries DB directly                     │
└──────────────────────┬──────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────┐
│              L1-services                        │  Business Logic Layer
│   Bridges UI requests to DB queries             │
│   Handles filtering, search, command building   │
│   (currently stubs — implementation pending)   │
└──────────────────────┬──────────────────────────┘
                       │
┌──────────────────────▼──────────────────────────┐
│               L0-core                           │  Foundation Layer
│   DatabaseManager  — reads tguide.db (SQLite)   │
│   UserDataManager  — reads/writes JSON files    │
│   ConfigManager    — reads/writes config.json   │
│   DBCacheManager   — integrity + hash tracking  │
│   PathResolver     — cross-platform paths       │
│   SHA256           — file integrity hashing     │
└─────────────────────────────────────────────────┘
```

**Rule:** Each layer only communicates with the layer directly below it. UI never touches `DatabaseManager` directly. `L0-core` never calls UI functions — it uses `extern` declarations for `UI_errors`, `UI_fatal`, and `UI_attention` to report errors without creating a circular dependency.

---

## 5. Database Architecture

### tguide.db (read-only, official data)

| Table | Purpose |
|-------|---------|
| `categories` | Category registry with display order (name, display_order, description) |
| `tools` | Security tools reference (name, category, short_desc, description, flags_all) |
| `tool_flags` | Individual flags per tool (name, description, protocols, root, loud) |
| `templates` | Command templates per tool (template_name, description, root, protocols, flag) |
| `vulnerabilities` | Known CVEs and exploits (name, metasploit_name, severity, service, platform, etc.) |
| `options` | Options per vulnerability (option_name, option_value) |
| `modules` | Metasploit/recon-ng modules (name, path, platform, type, API, mode) |

**Key design decisions:**
- `categories` table — controls display order and naming. Source of truth for the category browser. Managed exclusively via `data_adder`, never by the running application.
- `tools.category` — TEXT column storing the category name. No foreign key — keeps data_adder simple and avoids cascade issues on rename. A rename in `categories` propagates via `UPDATE tools SET category=? WHERE category=?` in data_adder.
- `tools.short_desc` — one-sentence summary shown in list views.
- `tools.description` — full multi-line explanation shown in detail view.
- `templates.description` — explains what the template accomplishes.

### user_data (writable, per-user JSON files)

| File | Contents |
|------|---------|
| `saved_commands.json` | `{ "commands": [{ id, tool_id, command, note }] }` |
| `saved_scripts.json` | `{ "scripts": [{ id, name, path, note }] }` |

**No SQLite for user data.** JSON was chosen because `json.hpp` is already bundled, the data volume is small, and the files are human-readable and editable without special tools.

---

## 6. File Paths Per Platform

| Resource | Linux | Termux | Windows | macOS |
|----------|-------|--------|---------|-------|
| Config | `~/.config/tguide/config.json` | `$PREFIX/etc/tguide/` | `%APPDATA%\tguide\` | `~/Library/Application Support/tguide/` |
| Database | `~/.local/share/tguide/tguide.db` | `$PREFIX/share/tguide/` | `%APPDATA%\tguide\` | `~/Library/Application Support/tguide/` |
| Saved commands | `~/.local/share/tguide/saved_commands.json` | `$PREFIX/share/tguide/` | `%APPDATA%\tguide\` | `~/Library/Application Support/tguide/` |
| Scripts folder | `~/.local/share/tguide/scripts/` | `$PREFIX/share/tguide/scripts/` | `%APPDATA%\tguide\scripts\` | `~/Library/Application Support/tguide/scripts/` |

**Note on Linux:** All paths are in user space — running tguide never requires root.
Installing via `.deb` (package manager) writes the DB to `/usr/share/tguide/tguide.db`,
which the app copies to `~/.local/share/tguide/` on first run. The install itself
requires sudo but running the tool does not.

---

## 7. Bootstrap Sequence (CoreRunner.cpp)

```
1. createDirs()           — create ~/.config/tguide, ~/.local/share/tguide (non-fatal on fail)
                            no root check — all paths are user space on every platform
4. ConfigManager          — load or create config.json with defaults
5. initColors()           — set ANSI toggle from config["colors"]
6. UserDataManager        — load saved_commands.json + saved_scripts.json
7. disclaimer check       — if config["disclaimer_accepted"] == 0: show disclaimer
8. DBCache::init()        — load .db_cache for integrity tracking
9. BackupManager::init()  — set backup path (dev-only, removed before release)
10. DB classes init       — VulnD, ModuD, ToolD, ToolFlagD, TemplateD
    └── resolveDatabase() called for each:
        Case 1: valid local DB, hash matches official → use it
        Case 2: valid DB, hash differs → prompt user
        Case 3a: DB at default path → move to config path
        Case 3b: no local DB → download from GitHub (HTTPS only, 30s timeout, 20MB max)
11. DBFatal() check       — abort if DB initialization failed
12. UIEngine::start()     — hand off to main UI loop
```

---

## 8. Database Integrity System

Every time the database is loaded, `resolveDatabase()` performs:

1. **Magic byte check** — first 16 bytes must match SQLite magic header
2. **Schema validation** — all 6 tables and their expected columns must exist
3. **SHA-256 hash check** — compared against `DB_OFFICIAL_HASH` in `db_cache_manager.h`
4. **Download verification** — downloaded files are re-hashed before acceptance

The `.db_cache` file records the current hash and a rolling history of the last 3 database states. On download, if the hash does not match the official release, the file is deleted and the program aborts.

---

## 9. Color System

Colors are ANSI escape codes defined in `UI_colors.h` as `constexpr` strings.

At runtime, `initColors(bool)` is called once from CoreRunner after config loads. On Windows and macOS, colors are always disabled regardless of config. On Linux and Termux, the `config["colors"]` value determines whether colors are active.

All output uses the pattern:
```cpp
cout << (colorsEnabled() ? Color::CYAN : "") << text
     << (colorsEnabled() ? Color::RESET : "");
```

---

## 10. Special Tools — Extended Menus

Two tools receive additional sub-menus beyond the standard tool detail screen:

### metasploit
When a user opens the metasploit tool, an extra option **"Vulnerabilities"** appears in the detail menu. This opens a dedicated sub-menu backed by the `vulnerabilities` and `options` tables in `tguide.db`.

### recon-ng
When a user opens the recon-ng tool, an extra option **"Modules"** appears in the detail menu. This opens a dedicated sub-menu backed by the `modules` table in `tguide.db`.

Both sub-menus share the same interaction pattern (search / filter / list all) but operate on different data and expose different filter fields. See `03_LOGIC_TABLE.md` for the full state breakdown.

---

## 11. Navigation System (Global)

The following keywords are accepted at **every prompt** in the application:

| Input | Action |
|-------|--------|
| `0` or `back` | Return to the previous screen |
| `q`, `quit`, `exit` | Close the program immediately (clean exit, no `exit()` call) |
| `next` or `n` | Go to next page (in paginated lists) |
| `prev` or `p` | Go to previous page (in paginated lists) |
| A number | Select item by index |
| An option name | Select item by name (case-insensitive) |

These are checked **before** any other input logic on every screen.

---

## 12. Developer Notes

**data_adder.cpp** is a standalone dev-only binary for populating `tguide.db`. It is excluded from `install()` targets in CMakeLists.txt and must be removed or disabled before public release.

**BackupManager** is dev-only infrastructure that copies the DB on every write operation. It will be removed before release along with all `add()` and `del()` methods on the DB classes.

**DB_OFFICIAL_HASH** and **DB_DOWNLOAD_URL** in `db_cache_manager.h` are intentionally empty during development. They must be set before the first public release.

---

## 13. Project Rules & Coding Standards

These rules apply to every file in the project without exception.

### Code Style

- C++17 throughout. No C++20 features — Termux compiler support varies.
- `#pragma once` in all headers. Never `#ifndef` guards.
- `using namespace std;` in `.cpp` files only. Never in headers.
- Member variables prefixed with `m_` (e.g., `m_page`, `m_items`).
- Section separators: `// ==================== NAME ====================`
- File header block on every file:
  ```cpp
  /*
   *  tguide — filename.cpp
   *  one-line description of what this file does
   *
   *  written by voidoxin
   */
  ```

### Comments

- Comments are for a solo developer maintaining the codebase long-term.
- One short line per function explaining *why*, not *what*.
- No over-documentation. No obvious comments like `// increment i`.
- English only. No Arabic. No AI-style verbose explanations.
- `// TODO:` markers for unimplemented stubs — always include what is needed.
- `// DEV-ONLY:` markers for code to be removed before release.

### Error Handling

- **Fatal errors** → `UI_fatal(msg)` then `return 1` in `main()`.
  Never followed by `clearScreen()`. Terminal output is the error record.
- **Recoverable errors** → `UI_errors(msg)` then `pause()` before next `clearScreen()`.
  User must see and acknowledge the message before it disappears.
- **Silent recovery** → auto-fix with no message (first-run file creation, default values).
- **Input errors** → inline message, re-prompt. No crash, no exit, no pause.
- `exit()` is **forbidden** everywhere. Always return through the call stack.
- Every function return value must be checked by the caller. No silent failures.

### Platform Compatibility

- All code compiles and runs on: Linux, Termux/Android, Windows, macOS.
- ANSI colors: Linux and Termux only. Windows and macOS: plain output.
- Never hardcode path separators (`/` or `\`). Use `std::filesystem` throughout.
- `getenv()` calls always check for null before use.
- `geteuid()` guarded by `#ifndef _WIN32`.
- `static_cast<unsigned char>` on all `isdigit()` / `isalpha()` calls.

### Input Handling

- All user input goes through `readInput()` from `UI_input.h`. Never `cin >>` directly.
- Every screen checks `isQuit()` then `isBack()` before any other logic.
- `readInput()` returns `""` on EOF — always treat as quit.
- Input is never assumed valid. Every possible value must have a defined code path.
- Long inputs (>256 chars) are truncated before processing.
- Non-ASCII and control characters survive `normalize()` if UTF-8, stripped if control.

### Navigation

- `0` or `back` → previous screen at any prompt.
- `q`, `quit`, `exit` → clean exit at any prompt.
- `next`/`n`, `prev`/`p` → page navigation in paginated lists.
- Numbers and option names both accepted. `matchOption()` handles ambiguity.
- Screen cleared on every navigation. Breadcrumb always visible.

### No Unexpected States

- Every function anticipates all return values including edge cases.
- Empty vectors, null pointers, -1 sentinels, false returns — all handled.
- No assumptions about database content (tables may be empty).
- No assumptions about filesystem state (files may disappear mid-run).
- No assumptions about user input (anything can be typed).

### Before Release Checklist

- [ ] Remove `data_adder.cpp` from build
- [ ] Remove `BackupManager` from all files
- [ ] Remove `add()` / `del()` methods from DB classes
- [ ] Set `DB_OFFICIAL_HASH` in `db_cache_manager.h`
- [ ] Set `DB_DOWNLOAD_URL` in `db_cache_manager.h`
- [ ] Populate `tguide.db` with full dataset using `data_adder`
- [ ] Test on Linux, Termux, Windows, macOS
- [ ] Verify disclaimer screen on first run
- [ ] Verify color toggle in settings

