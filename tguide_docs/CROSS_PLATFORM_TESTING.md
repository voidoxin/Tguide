# Cross-Platform Validation — tguide v1.0.1

**Author:** voidoxin  
**Read after:** `ROADMAP.md` (STEP-CP2)

---

## Overview

tguide targets 7 platforms. This document lists the platform-specific prerequisites,
build instructions, runtime verification steps, and known issues for each.

All code changes are tracked in `ROADMAP.md` under the B-series (cross-platform) steps.

---

## Platform Matrix

| Platform    | OS Variants        | Architecture | Build System   | Package Manager |
|-------------|--------------------|--------------|----------------|-----------------|
| Kali Linux  | Kali Rolling       | amd64        | cmake + gcc    | apt             |
| Ubuntu      | 22.04 / 24.04      | amd64        | cmake + gcc    | apt             |
| Fedora      | 39 / 40            | amd64        | cmake + gcc    | dnf             |
| Arch Linux  | Rolling            | amd64        | cmake + gcc/clang | pacman       |
| macOS       | Ventura+ / Sequoia+ | x86_64, arm64 | cmake + clang (Xcode) | Homebrew |
| Windows     | 10 / 11            | x86_64       | cmake + MSVC/clang-cl | vcpkg    |
| Termux      | Latest             | aarch64, armv7l | cmake + clang | pkg (Termux)   |

---

## Prerequisites by Platform

### Linux (Kali, Ubuntu, Fedora, Arch)

- C++17 compiler (GCC >= 9, Clang >= 10)
- cmake >= 3.16
- libcurl4-openssl-dev (Debian) / libcurl-devel (Fedora) / curl (Arch)
- sqlite3 (bundled, compiled from source)
- make or ninja

### macOS

- Xcode Command Line Tools (`xcode-select --install`)
- cmake (via Homebrew: `brew install cmake`)
- libcurl (bundled with macOS, but needs headers: `brew install curl`)
- C++17 support via AppleClang (Xcode >= 12)

### Windows

- Visual Studio 2022 (MSVC v143) OR clang-cl (via Visual Studio Installer)
- cmake >= 3.16 (via Visual Studio Installer or winget)
- vcpkg for CURL: `vcpkg install curl`
- C++17 standard

### Termux

- Install from F-Droid (not Google Play — Play version is outdated)
- `pkg install curl cmake ninja clang`
- Environment: `$PREFIX` = `/data/data/com.termux/files/usr`

---

## Build Instructions

### Linux / macOS

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
tguide
```

### Windows (MSVC)

```powershell
# From Visual Studio Developer Command Prompt or PowerShell with vcvars loaded
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build
tguide.exe
```

### Termux

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build
tguide
```

---

## Verification Checklist

For each platform, verify:

### 1. Compilation

- [ ] `cmake -B build` configures without errors
- [ ] `cmake --build build` completes without errors
- [ ] No warnings with default flags

### 2. Installation

- [ ] Binary installed to correct platform path
- [ ] Seed database installed to correct platform path
- [ ] No permission errors during install

### 3. Runtime — First Boot

- [ ] Application launches without crashes
- [ ] Database directories created (`~/.config/tguide`, `~/.local/share/tguide` on Linux)
- [ ] Disclaimer screen shown on first launch
- [ ] Accept disclaimer → enters main menu

### 4. Runtime — Offline

- [ ] Start without internet connection
- [ ] Warning shown, no crash
- [ ] Existing local database used successfully

### 5. Runtime — Online

- [ ] Start with internet connection
- [ ] Manifest fetch attempted (non-fatal)
- [ ] Tools menu shows categories
- [ ] Selecting a category shows tools
- [ ] Selecting a tool shows details/flags

### 6. Runtime — Search

- [ ] Search by tool name returns results
- [ ] Search with partial match works
- [ ] Empty search returns all results

### 7. Runtime — Generator

- [ ] Generator wizard opens
- [ ] Template selection works
- [ ] Placeholder substitution works
- [ ] Command preview shows correctly

### 8. Runtime — Saved Data

- [ ] Save a command works
- [ ] Save a script works
- [ ] View saved commands returns saved data
- [ ] Delete saved item works

### 9. Runtime — Settings

- [ ] Settings screen opens
- [ ] Color toggle works (Linux/macOS/Termux)
- [ ] Colors work in modern Windows Terminal (Windows 10/11)
- [ ] Colors off in legacy Windows cmd.exe
- [ ] DB update from settings works

### 10. Runtime — Color Support

- [ ] Terminal colors render correctly (Linux/macOS/Windows Terminal)
- [ ] No garble when piped (`./tguide | cat`)
- [ ] Colors off when stdout is redirected

### 11. Runtime — Ctrl+C Handling

- [ ] Ctrl+C during input prompt exits gracefully with "goodbye."
- [ ] Ctrl+C during recovery prompt exits gracefully
- [ ] No infinite loop on Ctrl+C at any prompt
- [ ] No crash dump or stack trace on Ctrl+C

### 12. Runtime — Input

- [ ] "q", "quit", "exit" at any prompt closes the app
- [ ] "0" or "back" returns to previous menu
- [ ] Number selection works throughout menus
- [ ] Invalid input shows warning and re-prompts

### 13. Cleanup

- [ ] `rm -rf ~/.config/tguide ~/.local/share/tguide` resets for clean re-test
- [ ] No leftover files in unexpected locations

---

## Platform-Specific Notes

### macOS

- Install path for seed DB: `/Library/Application Support/tguide/tguide.db`
- User paths: `~/Library/Application Support/tguide/`
- Must use `sudo cmake --install` for system installation
- AppleClang should honor C++17 via CMake settings

### Windows

- Install paths via `cmake --install`: `%PROGRAMDATA%/tguide/`
- Runtime paths: `%APPDATA%/tguide/` (user) / `%PROGRAMDATA%/tguide/` (system seed)
- ANSI colors: `SetConsoleMode(ENABLE_VIRTUAL_TERMINAL_PROCESSING)` — works in Windows Terminal
- Legacy cmd.exe: colors fall back to off (no garble)
- Ctrl+C: handled via `SetConsoleCtrlHandler`
- Building requires `vcpkg install curl` for CURL dependency
- `NOMINMAX` and `_CRT_SECURE_NO_WARNINGS` defined in CMake for MSVC

### Termux

- Detectable via `__ANDROID__` preprocessor define
- Paths: `HOME/../usr/etc/tguide/` (config), `HOME/../usr/share/tguide/` (data)
- Colors enabled via standard ANSI (Termux supports VT processing)
- Install paths: `$PREFIX/bin/`, `$PREFIX/share/tguide/`

---

## Known Issues and Fixes

| #  | Platform   | Issue                                                           | Status     | Fix                                        |
|----|------------|-----------------------------------------------------------------|------------|--------------------------------------------|
| 1  | All        | C++17 required (`std::filesystem`)                              | ✅ Fixed   | CMake requires C++17                       |
| 2  | macOS      | `AppleClang` needs proper compiler flag detection               | ✅ Fixed   | CMake generator expression includes AppleClang |
| 3  | Windows    | MSVC uses `/Od` instead of `-O0`                                | ✅ Fixed   | STEP-B3a — generator expression            |
| 4  | Windows    | `windows.h` min/max macros break C++                            | ✅ Fixed   | STEP-B3a — NOMINMAX define                 |
| 5  | Windows    | ANSI colors disabled on Windows                                 | ✅ Fixed   | STEP-B3b — SetConsoleMode(VT processing)   |
| 6  | Windows    | Ctrl+C caused abrupt exit                                       | ✅ Fixed   | STEP-B3b — SetConsoleCtrlHandler           |
| 7  | Windows    | `unistd.h` not available on MSVC                                | ✅ Fixed   | Guarded with `_WIN32` in UI_colors.h       |
| 8  | Termux     | Runtime `isTermux()` detection not reliable at compile time     | ✅ Fixed   | STEP-B2b — `__ANDROID__` preprocessor guard |
| 9  | All        | Cross-platform path resolution for config/data                  | ✅ Fixed   | STEP-B2a/B2b — XDG + platform guards       |
| 10 | All        | Database bootstrap without internet                             | ✅ Fixed   | STEP-B1b/B1c/B1d — seed DB + non-fatal     |

---

## Test Execution Record

| Platform          | Date       | Tester  | Compilation | Runtime | Issues Found |
|-------------------|------------|---------|-------------|---------|--------------|
| Kali Linux        | -          | -       | -           | -       | -            |
| Ubuntu            | -          | -       | -           | -       | -            |
| Fedora            | -          | -       | -           | -       | -            |
| Arch Linux        | -          | -       | -           | -       | -            |
| macOS (Intel)     | -          | -       | -           | -       | -            |
| macOS (Apple Silicon) | -      | -       | -           | -       | -            |
| Windows 10        | -          | -       | -           | -       | -            |
| Windows 11        | -          | -       | -           | -       | -            |
| Termux            | -          | -       | -           | -       | -            |
