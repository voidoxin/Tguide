# Tguide — Development Roadmap
## Version: 1.4.0
## Date: 2026-06-07
## Author: voidoxin

## Current State (from code scan)
- **Proposals reviewed and accepted (2026-06-07)**: Four proposals from `tguide_technical_proposal.md` were independently reviewed by software-architect, security-auditor, and developer-core via agent consensus. **P1 (Dynamic Version Manifest)** and **P4 (DB Rollback/Recovery)** approved as a unified DB lifecycle subsystem (highest priority, replaces STEP-11 hardcoded-hash approach). **P3 (GitHub Threat Model)** approved as documentation (low priority). **P2 (DNS-Only Check)** deferred to Phase 3 (medium priority, future). New steps STEP-P1, STEP-P4, STEP-P3, and STEP-P2 incorporated into this roadmap.
- **STEP-P1 complete (2026-06-08)**: Dynamic version manifest implemented. DBResolver fetches signed_manifest.json from GitHub, parses version/db_hash/db_url, validates hash before download acceptance. DBCacheManager persists last_seen_version. DB_OFFICIAL_HASH removed from compiled binary.
- **CoreRunner.cpp**: Bootstrap sequence mostly implemented, root check bug (BUG-1) fixed (STEP-00), dev-only BackupManager remains
- **L0-core**: DatabaseManager has basic CRUD, UserDataManager created but not integrated, path_resolver needs Linux user-space fixes, DBCacheManager needs official hash; **A1 architectural violation fixed** (UI→L0 direct includes removed); **A2 architectural violation fixed** (raw extern callbacks → ErrorHandler abstraction); **A3 architectural violation fixed** (Global Mutable State eliminated: DBResolver, DBCacheManager, g_colorEnabled); **A-03 security fix completed** (7 SSL/TLS hardening measures in DBResolver.cpp, commit 4c818c2)
- **L1-services**: svc_tools has clearSearchIndex (STEP-03), svc_generator has sanitizeInput (STEP-04), **A6 architectural violation fixed** (L1 DTOs decouple L2 from L0 types); remaining service files still stubbed or minimal
- **L2-Interface_Engine**: UI_Engine uses readInput() (STEP-01), toNumber() uses from_chars (STEP-02), UI_colors checks isatty() (STEP-05), but tool detail, saved commands/scripts, settings, and generator still stubbed
- **CMakeLists.txt**: Build system configured across platforms but installs data_adder (dev-only) and lacks release configuration; **A7 architectural violation fixed** (build-time layer enforcement: L3_interface no longer includes/link L0_core directly); **A5 partial fix** (dead L2_INC ref removed from L3_interface)
- **Missing**: Database schema updates (short_desc), complete UserDataManager integration, search algorithm, filter implementations, saved data screens, script generator, settings screen, and dev-only code removal
- **Completed analysis**: `.ai/dependencies.md` created (388 lines, cataloging 6 dependencies); cross-platform compatibility analysis completed and **STEP-CP1 complete** — macOS cross-platform fixes applied (path resolution, ANSI colors, CMake install targets, libcurl RAII guard); **A-06 test infrastructure complete** — doctest single-header framework, 21 test cases across 3 modules (SHA256, ConfigManager, UserDataManager), TempDirectory/TempFile/ErrorHandlerSpy fixtures, CMake/CTest integration (commit 8ae3447)
- **STEP-08 complete (2026-06-07)**: UserDataManager now uses Meyer's singleton pattern. CoreRunner properly integrates it — the instance survives beyond bootstrap. Ready for Phase 4 service layer.
- **STEP-10 complete (2026-06-07)**: Linux root requirement eliminated. All runtime paths moved from `/etc/tguide` and `/usr/share/tguide` to `~/.config/tguide` and `~/.local/share/tguide`. The tool no longer requires root on any platform.
- **STEP-07 complete (2026-06-07)**: Database schema updated — Category struct, categories table, CategoryD CRUD class added to L0-core. Interactive category management in data_adder menu.

## Architecture Reference
```
L0-core → L1-services → L2-Interface_Engine
```
- L0 handles database, config, user data, paths, and integrity
- L1 bridges UI to DB, implements search, filtering, command building
- L2 handles all UI, input, menus, colors, and navigation
- Layers only communicate downward (L2→L1→L0), no circular dependencies

## Phase 0 — Critical Bug Fixes
### STEP-00 — Fix root check in CoreRunner.cpp
| Field      | Value |
|------------|-------|
| Layer      | BUGFIX |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | CoreRunner.cpp |
| Goal       | Remove Linux root requirement by removing hasWriteAccess() check and related fatal error |
| Depends    | none |
| Done when  | CoreRunner.cpp no longer contains hasWriteAccess() check or UI_fatal for root privileges |

### STEP-01 — Fix direct cin >> usage in UI_Engine.cpp
| Field      | Value |
|------------|-------|
| Layer      | BUGFIX |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_Engine.cpp |
| Goal       | Replace all direct cin >> usage with readInput() from UI_input.h |
| Depends    | STEP-00 |
| Done when  | UI_Engine.cpp uses readInput() exclusively for all input operations |

### STEP-02 — Fix stoi crash risk in toNumber()
| Field      | Value |
|------------|-------|
| Layer      | BUGFIX |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_input.cpp |
| Goal       | Replace std::stoi with std::from_chars or try/catch block for safe integer conversion |
| Depends    | STEP-01 |
| Done when  | toNumber() function handles invalid input without throwing exceptions |

### STEP-03 — Fix search index memory leak
| Field      | Value |
|------------|-------|
| Layer      | BUGFIX |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L1-services/src/svc_tools.cpp |
| Goal       | Add index clearing (s_index.index.clear() and s_index.tools.clear()) before database reload in shadow swap scenarios |
| Depends    | STEP-02 |
| Done when  | svc_tools.cpp contains code to clear search index before database updates |

### STEP-04 — Fix command injection risk in script generation
| Field      | Value |
|------------|-------|
| Layer      | BUGFIX |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L1-services/src/svc_generator.cpp, L2-Interface_Engine/src/UI_generator.cpp |
| Goal       | Implement sanitizeInput() function to block ; | & $ > < backticks and use std::quoted for paths |
| Depends    | STEP-03 |
| Done when  | All user inputs in script generation are sanitized before command construction |

### STEP-05 — Fix isatty() missing for non-interactive shells
| Field      | Value |
|------------|-------|
| Layer      | BUGFIX |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_colors.h, L0-core/src/config_manager.cpp |
| Goal       | Add isatty() check to automatically disable colors when output is not a terminal |
| Depends    | STEP-04 |
| Done when  | Colors are automatically disabled when stdout is not a terminal (checked via isatty()) |

### STEP-A1 — Fix A1 architectural violation (UI→L0 direct includes)
| Field      | Value |
|------------|-------|
| Layer      | L2 (Architecture) |
| Priority   | CRITICAL |
| Status     | [x] DONE (June 2026) |
| Files      | L2-Interface_Engine/src/UI_tools.cpp |
| Goal       | Remove direct L0-core includes from UI layer; `Tool` type accessed transitively via `svc_tools.h` (L1) → `DatabaseManager.h` (L0) |
| Depends    | none |
| Done when  | No `#include "../../L0-core/"` directives remain in `L2-Interface_Engine/src/*.cpp` |
| Completed  | **2026-06-05** — Removed `#include "../../L0-core/include/DatabaseManager.h"` (REDUNDANT: `Tool` type available via `svc_tools.h`) and `#include "../../L0-core/include/path_resolver.h"` (UNUSED: no PathResolver symbols referenced). 0 L0-core includes remain in `L2-Interface_Engine/src/*.cpp`. 10 `Tool` usages still compile transitively. Function signatures simplified (no more `ToolD& db` parameters). Code review: APPROVED ✅. Test strategy: 6 manual test cases in TEST_REPORT.md. Known remaining: `UI_disclaimer.h` still includes `config_manager.h` (tracked as separate violation). |

### STEP-A2 — Fix A2 architectural violation (extern callbacks → ErrorHandler)
| Field      | Value |
|------------|-------|
| Layer      | L0 (Architecture) |
| Priority   | CRITICAL |
| Status     | [x] DONE (June 2026) |
| Files      | L0-core/include/ErrorHandler.h (new), L0-core/src/ErrorHandler.cpp (new), L0-core/include/DatabaseManager.h (modified), L0-core/src/DatabaseManager.cpp (modified), L0-core/src/UserDataManager.cpp (modified), CoreRunner.cpp (modified), CMakeLists.txt (modified) |
| Goal       | Replace raw extern function pointers (g_onFatal/g_onError/g_onAttention) declared in L0-core with a proper ErrorHandler struct using std::function to eliminate the reverse dependency from L0→L2 |
| Depends    | STEP-05 |
| Done when  | No occurrences of g_onFatal/g_onError/g_onAttention remain; g_errorHandler used consistently across 18 sites (declaration, definition, 15 call sites, 1 assignment) |
| Completed  | **2026-06-05** — ErrorHandler struct created with std::function members. Extern declarations removed from DatabaseManager.h. All 15 call sites updated in DatabaseManager.cpp and UserDataManager.cpp. CoreRunner.cpp uses aggregate initialization. ErrorHandler.cpp added to CMakeLists.txt L0_SOURCES. 0 occurrences of old g_onFatal/g_onError/g_onAttention remain. Code review: APPROVED ✅. Test strategy: 26 manual test cases in TEST_REPORT.md. **Post-completion regression fix**: Restored `if (binder) binder(stmt);` call in `DBSession::query()` (line 78 of DatabaseManager.cpp) that was inadvertently dropped during the ErrorHandler refactor. Without this call, all parameterized SQL queries silently returned unfiltered results because binding never executed. A1: also removed `#include "DatabaseManager.h"` from `UserDataManager.cpp` (unused). Code review: APPROVED ✅. Testing: PASS ✅. |

### STEP-A3 — Fix A3 architectural violation (Global Mutable State)
| Field      | Value |
|------------|-------|
| Layer      | L0+L1+L2 (Architecture) |
| Priority   | MEDIUM |
| Status     | [x] DONE (June 2026) |
| Files      | DBResolver.h (NEW), DBResolver.cpp (NEW), DatabaseManager.cpp, DatabaseManager.h, db_cache_manager.h, db_cache_manager.cpp, UI_colors.h, svc_tools.h, svc_tools.cpp, CoreRunner.cpp, CMakeLists.txt |
| Goal       | Eliminate all 4 sites of global mutable state: DBContext (file-scoped struct), DBCache (file-scoped namespace globals), SearchIndex (Meyer's singleton, dead code), colorFlag (function-local static bool). Replace with resettable singleton accessors (DBResolver, DBCacheManager) or inline variables (g_colorEnabled). |
| Depends    | STEP-A2 (ErrorHandler must exist first for DBResolver error reporting) |
| Done when  | No static/global mutable state remains except g_errorHandler (intentionally global). DBResolver and DBCacheManager use instance() singleton with resetForTesting() behind #ifndef NDEBUG. SearchIndex removed entirely. colorFlag() replaced with inline bool g_colorEnabled. |
| Completed  | **2026-06-05** — Four sites: S3 (SearchIndex removed from svc_tools.cpp/h — dead code, zero callers), S4 (colorFlag() → inline bool g_colorEnabled in UI_colors.h — identical semantics, zero call-site changes), S2 (namespace DBCache → class DBCacheManager in db_cache_manager.h/cpp — file-scoped statics moved to members, instance() singleton, 4 call sites updated), S1 (DBContext struct + resolveDatabase() → class DBResolver in new DBResolver.h/cpp — extracted from DatabaseManager.cpp, 5 DB constructors use instance().resolve(), DBFatal() delegates to instance().fatal(), downloadDB() kept private, resetForTesting() guarded by #ifndef NDEBUG, 5 dead includes removed). Security audit: APPROVED (fixes applied). Code review: APPROVED (fixes applied). Testing: PASS ✅. |

### STEP-A7 — Fix A7 architectural violation (No Build-Time Layer Enforcement)
| Field      | Value |
|------------|-------|
| Layer      | CMake (Architecture) |
| Priority   | CRITICAL |
| Status     | [x] DONE (June 2026) |
| Files      | CMakeLists.txt |
| Goal       | Enforce L3→L1→L0 dependency chain at build time: remove L0 include directories and link dependency from L3_interface; make L0 headers PRIVATE to L1_services so they don't leak to consumers |
| Depends    | none |
| Done when  | L3_interface cannot directly include or link L0_core at build time; `target_include_directories(L1_services ...)` split into PUBLIC (own headers) + PRIVATE (L0 headers); `target_include_directories(L3_interface ...)` contains only L3_INC and L1_INC; `target_link_libraries(L3_interface ...)` links only L1_services |
| Completed  | **2026-06-05** — Three changes applied to CMakeLists.txt: (1) L1_services include directories split: `PUBLIC ${L1_INC}` + `PRIVATE ${L0_INC} ${LIBS_DIR}` to prevent L0 header leakage; (2) L3_interface include directories stripped: removed `${L2_INC}` (dead ref — **partial A5 fix**), `${L0_INC}` (violation), `${LIBS_DIR}` (violation) — only `${L3_INC}` and `${L1_INC}` remain; (3) L3_interface link libraries changed from `PRIVATE L0_core PRIVATE L1_services` to `PRIVATE L1_services` — only links the layer below. Targets NOT modified: `data_adder` (dev tool — exempt), `tguide` (bootstrap — needs all layers, still links L0_core directly), `L0_core` (foundation — already correct). Existing cross-layer includes use relative paths (not include directories), so existing code compiles. Static library allows undefined symbols; tguide executable resolves all L0 symbols. Code review: APPROVED ✅. Test strategy: 7 test cases in TEST_REPORT.md (cmake configure, build, source-level include grep, symbol-level nm, manual regression, enforcement verification, rollback plan). |

### STEP-A4 — Fix A4 architectural violation (Disclaimer bypasses L1)
| Field      | Value |
|------------|-------|
| Layer      | L2 (Architecture) |
| Priority   | LOW |
| Status     | [x] DONE (June 2026) |
| Files      | L2-Interface_Engine/includes/UI_disclaimer.h (modified), L2-Interface_Engine/src/UI_disclaimer.cpp (modified) |
| Goal       | Remove direct L0-core header include from L2 header. L2 headers must not expose L0 types; L2 .cpp files may include L0 headers internally. |
| Depends    | none |
| Done when  | UI_disclaimer.h no longer includes any L0-core header; ConfigManager is forward-declared; UI_disclaimer.cpp includes config_manager.h directly |
| Completed  | **2026-06-05** — Removed `#include "../../L0-core/include/config_manager.h"` from `UI_disclaimer.h`, replaced with `class ConfigManager;` forward declaration. Added `#include "../../L0-core/include/config_manager.h"` to `UI_disclaimer.cpp`. Security audit: APPROVED ✅. Code review: APPROVED ✅. Testing: PASS ✅. |

### STEP-A6 — Fix A6 architectural violation (No DTOs in L1)
| Field      | Value |
|------------|-------|
| Layer      | L1 (Architecture) |
| Priority   | MEDIUM |
| Status     | [x] DONE (June 2026) |
| Files      | L1-services/includes/svc_dto.h (new), L1-services/includes/svc_tools.h (modified), L1-services/src/svc_tools.cpp (modified), L2-Interface_Engine/src/UI_tools.cpp (modified) |
| Goal       | Create Data Transfer Objects (DTOs) in L1 to decouple L2 UI from L0-core data types. L1 headers must not expose L0 types in their public API. |
| Depends    | STEP-A2 |
| Done when  | L1 headers expose only L1-owned DTO types; L2 UI code references only SvcDTO::ToolDTO, never L0::Tool; L0 includes are confined to L1 .cpp files |
| Completed  | **2026-06-05** — Created `svc_dto.h` with 6 DTOs (ToolDTO, VulnerabilityDTO, ModuleDTO, ToolFlagDTO, TemplateDTO, OptionDTO) in `namespace SvcDTO`. Removed `#include "../../L0-core/include/DatabaseManager.h"` from `svc_tools.h` (now includes `svc_dto.h`). Changed `getToolsByCategory()` and `searchTools()` return types from `std::vector<Tool>` to `std::vector<SvcDTO::ToolDTO>`. Added static mapper functions (`toDTO`, `toDTOs`) in `svc_tools.cpp`. Updated all 4 `Tool` references in `UI_tools.cpp` to `SvcDTO::ToolDTO`. Architecture review: APPROVED ✅. Code review: APPROVED ✅. Testing: PASS ✅. |

### STEP-A8 — Fix A8 architectural violation (ErrorHandler timing hole)
| Field      | Value |
|------------|-------|
| Layer      | BOOTSTRAP (Architecture) |
| Priority   | MEDIUM |
| Status     | [x] DONE (June 2026) |
| Files      | CoreRunner.cpp |
| Goal       | Fix error handling bypass: g_errorHandler was registered too late (after UserDataManager and PathResolver had already run), causing errors from those components to be silently swallowed via null std::function members. |
| Depends    | STEP-A2 (ErrorHandler must exist first) |
| Done when  | g_errorHandler registration is moved before any L0-core calls; UserDataManager error paths (lines 21, 70, 95) execute with valid handler |
| Completed  | **2026-06-05** — Moved `g_errorHandler = { UI_fatal, UI_errors, UI_attention }` from line 73 to line 25 of CoreRunner.cpp, immediately after `int main() {` and before any L0 calls. Now covers UserDataManager (lines 54-58), PathResolver (lines 33-42), and ConfigManager (line 45). Comment explains the bug class (null std::function swallowing errors). Security audit: APPROVED ✅. Code review: APPROVED ✅. Testing: PASS ✅. |

### STEP-A9 — Fix A9 architectural violation (CoreRunner over-instantiation)
| Field      | Value |
|------------|-------|
| Layer      | BOOTSTRAP (Architecture) |
| Priority   | MEDIUM |
| Status     | [x] DONE (June 2026) |
| Files      | CoreRunner.cpp |
| Goal       | Remove 5 redundant DatabaseManager subclass instantiations (VulnD, ModuD, ToolD, ToolFlagD, TemplateD) at boot that were never used — service layer creates its own instances. Replace with single ToolD bootstrap check to preserve database error detection. |
| Depends    | none |
| Done when  | CoreRunner.cpp contains only one DB bootstrap check instead of 5; DBFatal() guard preserved; all 5 unused variables removed |
| Completed  | **2026-06-05** — Removed `VulnD vulnDB(db_path)`, `ModuD moduDB(db_path)`, `ToolD toolDB(db_path)`, `ToolFlagD flagDB(db_path)`, `TemplateD tmplDB(db_path)` and the temporary `db_path` variable. Added `ToolD _bootCheck(PathResolver::dbFile().string()); (void)_bootCheck;` as a single bootstrap check. DBFatal() guard preserved unchanged. Security audit: APPROVED ✅. Code review: APPROVED ✅. Testing: PASS ✅. |

### STEP-A03 — Fix libcurl SSL/TLS verification in DBResolver.cpp (A-03)
| Field      | Value |
|------------|-------|
| Layer      | L0 (Security) |
| Priority   | CRITICAL |
| Status     | [x] DONE (June 2026) |
| Files      | L0-core/src/DBResolver.cpp |
| Goal       | Add 7 SSL/TLS security hardening measures to DBResolver::downloadDB(): enforce certificate chain validation, strict hostname matching, TLS v1.2 minimum, HTTPS-only protocol restriction, redirect chain capped at 5, and SSL-specific error differentiation |
| Depends    | none |
| Done when  | DBResolver.cpp configures curl with CURLOPT_SSL_VERIFYPEER=1, CURLOPT_SSL_VERIFYHOST=2, CURLOPT_SSLVERSION=TLSv1_2, CURLOPT_PROTOCOLS_STR="https", CURLOPT_REDIR_PROTOCOLS_STR="https", CURLOPT_MAXREDIRS=5, and SSL error differentiation via g_errorHandler |
| Completed  | **2026-06-06** — 7 SSL/TLS hardening measures added to `DBResolver::downloadDB()` (commit 4c818c2). Includes `LIBCURL_VERSION_NUM >= 0x075500` guard for curl < 7.85.0 backward compatibility (bitmask fallback via `CURLOPT_PROTOCOLS`/`CURLOPT_REDIR_PROTOCOLS`). Three SSL error codes (CURLE_SSL_CONNECT_ERROR, CURLE_PEER_FAILED_VERIFICATION, CURLE_SSL_CERTPROBLEM) now produce contextual error messages via `g_errorHandler`. Security audit: APPROVED. Code review: APPROVED. Testing: PASS. |

### STEP-CP1 — macOS cross-platform portability fixes
| Field      | Value |
|------------|-------|
| Layer      | CROSS-PLATFORM |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L0-core/include/path_resolver.h, data_adder.cpp, CoreRunner.cpp, CMakeLists.txt, L2-Interface_Engine/includes/UI_colors.h, .ai/dependencies.md |
| Goal       | Implement macOS compatibility: use `~/Library/Application Support/tguide/` paths, fix include paths in data_adder, add curl_global_init/cleanup RAII, add macOS CMake install targets and CPack Bundle support, fix CURL::libcurl PUBLIC propagation, enable ANSI colors on macOS, update dependency docs |
| Depends    | STEP-05 (isatty fix enables ANSI color toggle), STEP-A03 (curl SSL context) |
| Done when  | tguide compiles and runs on macOS without root; paths resolved under ~/Library/Application Support/tguide/; ANSI colors working in Terminal.app; `cmake --install build` installs binary to /usr/local/bin |
| Completed  | **2026-06-06** — Six files modified across the codebase: (1) `path_resolver.h`: Added `__APPLE__` guards in `configDir()`, `dataDir()`, `userDataDir()`, and `hasWriteAccess()` — all returning writable paths under `~/Library/Application Support/tguide/` with no root required. (2) `data_adder.cpp`: Fixed include paths (lines 4, 6) to use CMake include directories instead of relative paths. (3) `CoreRunner.cpp`: Added `curl_global_init(CURL_GLOBAL_DEFAULT)` before all L0 calls and `CurlGuard` RAII struct calling `curl_global_cleanup()` on destruction; updated error message. (4) `CMakeLists.txt`: Added macOS platform detection (`IS_MACOS`), macOS install target (binary → `/usr/local/bin`), macOS CPack Bundle generator, and fixed `CURL::libcurl` propagation (`PRIVATE` → `PUBLIC` on `L0_core`). (5) `UI_colors.h`: Removed `__APPLE__` from color guard — `#else` branch now covers macOS, Linux, and Termux, enabling ANSI colors in macOS Terminal.app. (6) `.ai/dependencies.md`: Updated libcurl section to reflect `CoreRunner.cpp` usage of `curl_global_init/cleanup` and `PUBLIC` visibility of `CURL::libcurl`. Key outcomes: no root required on macOS, paths unified under `~/Library/Application Support/tguide/`, easy installation via `cmake --build build && cmake --install build`, ANSI colors working in macOS Terminal.app. Code review: APPROVED ✅. Testing: PASS ✅. |

### STEP-A06 — Add minimum viable test infrastructure with doctest
| Field      | Value |
|------------|-------|
| Layer      | TEST |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | libs/doctest.h (NEW), tests/main.cpp (NEW), tests/fixtures.h (NEW), tests/test_sha256.cpp (NEW), tests/test_config_manager.cpp (NEW), tests/test_user_data_manager.cpp (NEW), tests/CMakeLists.txt (NEW), CMakeLists.txt (modified), .ai/dependencies.md (modified) |
| Goal       | Add minimum viable test infrastructure using the doctest single-header framework to enable automated unit testing across all layers. |
| Depends    | none |
| Done when  | Tests compile and pass with CTest: 21 test cases across 3 modules (SHA256: 8, ConfigManager: 6, UserDataManager: 7). Test fixtures include TempDirectory, TempFile, and ErrorHandlerSpy. CTest integration via `cmake --build build && ctest --test-dir build`. |
| Completed  | **2026-06-06** — doctest single-header framework (v2.4.11, 9119 lines) added to `libs/doctest.h`. Test entry point in `tests/main.cpp`. Three fixture types defined in `tests/fixtures.h`: `TempDirectory` (auto-cleanup temp dirs), `TempFile` (scoped file with content), `ErrorHandlerSpy` (op-count error handler spy). Three test modules totaling 21 cases: `test_sha256.cpp` (8 cases: empty, short, known vectors, collision detection, incremental, streaming), `test_config_manager.cpp` (6 cases: defaults, load/save, get/set, non-existent load, save on destroy), `test_user_data_manager.cpp` (7 cases: init, save/load, add/remove command, add/remove script, persistence, clear, empty state). CMake target `tguide_tests` built with `target_link_libraries(tguide_tests PRIVATE L0_core)` and registered with `add_test()`. Top-level `CMakeLists.txt` updated with `enable_testing()` and `add_subdirectory(tests)`. `.ai/dependencies.md` updated with doctest section (§6). All tests pass cleanly. Code review: APPROVED ✅. Testing: PASS ✅ (21/21 passing). Commit: 8ae3447. |

## Phase 1 — Foundation (L0 only)
### STEP-06 — Fix error handling contract implementation (TASK-1b)
| Field      | Value |
|------------|-------|
| Layer      | L0+L1+L2 |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_errorHandling.cpp, L2-Interface_Engine/includes/UI_errorHandling.h, L2-Interface_Engine/src/UI_Engine.cpp (audit all files) |
| Goal       | Implement professional error handling system: UI_fatal() never followed by clearScreen(), UI_errors() always followed by pause() before clearScreen |
| Depends    | STEP-05 |
| Done when  | All UI_errors() calls are followed by pause() before clearScreen() and UI_fatal() is never followed by clearScreen() |
| Completed  | **2026-06-07** — Audit of all 12 error-handling call sites complete. No contract violations found. Added `noexcept` to `waitForEnter()`, `UI_fatal()`, `clearScreen()`, `readInput()`. Changed `printInvalidInput()` in UI_Engine.cpp to use `Color::YELLOW` with `colorsEnabled()` guard. Fixed include ordering to match coding style (standard → project). All changes reviewed and tested via fix/check loop: code-reviewer APPROVED ✅, test-engineer PASS ✅. |

### STEP-07 — Update database schema for short_desc and categories table
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L0-core/include/DatabaseManager.h, L0-core/src/DatabaseManager.cpp, L0-core/src/DBResolver.cpp, data_adder.cpp |
| Goal       | Add short_desc column to Tool struct and create CategoryD class with categories table schema |
| Depends    | STEP-06 |
| Done when  | Database schema includes short_desc in Tool struct and categories table with id, name, display_order, description columns |
| Completed  | **2026-06-07** — Category struct, CategoryResults, and CategoryD class added to DatabaseManager.h. CategoryD implemented with SAFE_CATEGORY_COLS whitelist in DatabaseManager.cpp (lines 719-801). Categories added to DBResolver.cpp schema validation (line 82) and dbHasData() (line 116). Categories CRUD (Add/View/Delete) added to data_adder.cpp menu (options 18-20). Code review: APPROVED ✅. Dependencies check: APPROVED ✅. |

### STEP-08 — Implement UserDataManager for saved commands and scripts
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L0-core/include/UserDataManager.h, L0-core/src/UserDataManager.cpp, CoreRunner.cpp, L0-core/include/path_resolver.h |
| Goal       | Create UserDataManager class to handle saved_commands.json and saved_scripts.json, integrate into CoreRunner, and extend PathResolver with user data paths |
| Depends    | STEP-07 |
| Done when  | UserDataManager loads/saves JSON files and CoreRunner constructs it after config load |
| Completed  | **2026-06-07** — Made UserDataManager a singleton (DBResolver/DBCacheManager pattern) with `instance()` + `init()`. Added `m_initialized` guard to all mutating methods. Changed CoreRunner to use `UserDataManager::instance().init(...)` instead of local variable — the singleton now survives beyond bootstrap and is accessible to L1 services in Phase 4. All 6 existing UserDataManager tests pass. |

### STEP-09 — Add I18n / Localization Framework foundation (NEW-1)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | HIGH |
| Status     | [x] DONE |
| Files      | L0-core/include/strings.h, L0-core/src/strings.cpp, L2-Interface_Engine/includes/UI_tools.h (sample conversion) |
| Goal       | Create string table system with enum-based string IDs and get() function for externalizing all UI text |
| Depends    | STEP-08 |
| Done when  | At least one UI screen uses get(STRING_ID) instead of hardcoded strings |
| Completed  | **2026-06-08** — StringID enum (8 entries) + Strings::get() in new L0-core/include/strings.h and L0-core/src/strings.cpp. Bounds-safe accessor with assert + static_assert. UITools::show() converted to use Strings::get() — menu labels, status messages, and prompt all externalized. 3 new doctest cases covering non-empty validation, content verification, and reference stability. All 3 new tests pass. Build compiles clean with zero warnings. |

### STEP-10 — Fix path resolver for Linux user-space only (no root required)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L0-core/include/path_resolver.h, CoreRunner.cpp |
| Goal       | Update path resolver to use user-space paths only on Linux and make hasWriteAccess() always return true |
| Depends    | STEP-09 |
| Done when  | Linux paths point to ~/.config/tguide/ and ~/.local/share/tguide/ instead of /etc/ and /usr/share/ |
| Completed  | **2026-06-07** — Fast-tracked before STEP-07/08/09. Changed `configDir()` on Linux from `/etc/tguide` to `~/.config/tguide` via `$HOME` env var. Changed `dataDir()` on Linux from `/usr/share/tguide` to `~/.local/share/tguide`. Removed `isRoot()`, `hasWriteAccess()`, and `#include <unistd.h>` (dead code). Updated CoreRunner.cpp comment. Updated CMakeLists.txt install paths to `/usr/local/...` with corrected comments. Dependencies check: APPROVED ✅. Code review: APPROVED ✅. |

### STEP-P1 — Implement Dynamic Version Manifest (replaces STEP-11)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | CRITICAL |
| Status     | [x] DONE |
| Files      | L0-core/src/DBResolver.cpp, L0-core/include/DBResolver.h, L0-core/include/db_cache_manager.h, L0-core/src/db_cache_manager.cpp, CoreRunner.cpp, .ai/security.md |
| Goal       | Replace compile-time DB_OFFICIAL_HASH with an HTTPS-fetched JSON manifest (signed_manifest.json) listing known-good DB versions + SHA256 hashes. Add fetchManifest() to DBResolver; store last-seen version in .db_cache; remove DB_OFFICIAL_HASH constant; anchor trust in HTTPS transport layer. |
| Depends    | STEP-10 (DONE — unblocked) |
| Done when  | DBResolver fetches manifest.json over HTTPS, parses version/db_hash/db_url, downloads DB from manifest URL, verifies hash, and accepts or rejects accordingly. DB_OFFICIAL_HASH no longer exists in compiled binary. .ai/security.md updated with manifest system description. |
| Completed  | **2026-06-08** — Dynamic version manifest implemented. DBResolver fetches signed_manifest.json from GitHub, parses version/db_hash/db_url, validates hash before download acceptance. DBCacheManager persists last_seen_version. DB_OFFICIAL_HASH removed from compiled binary. |

### STEP-P4 — Implement Database Rollback & Recovery System
| Field      | Value |
|------------|-------|
| Layer      | L0 + L2 |
| Priority   | HIGH |
| Status     | [x] DONE |
| Files      | L0-core/src/DBResolver.cpp, L0-core/include/DBResolver.h, L0-core/include/db_cache_manager.h, L0-core/src/db_cache_manager.cpp, CoreRunner.cpp, L2-Interface_Engine/src/UI_settings.cpp, L2-Interface_Engine/includes/UI_settings.h, tests/test_db_recovery.cpp, tests/CMakeLists.txt |
| Goal       | Implement N-1 auto-backup (tguide.db.bak) before every DB update; detect corruption on boot via schema validation; three-state recovery prompt (Restore/Keep/Ask Later); manual "Rollback Database" button in Settings → Database screen. |
| Depends    | STEP-P1 |
| Done when  | Active DB is backed up to tguide.db.bak before each update; boot validation triggers recovery prompt on corruption with three options (Restore/Keep/Ask Later); Settings screen shows active version, backup availability, and rollback/delete-backup options. |
| Completed  | **2026-06-08** — Backup before download: DBResolver backs up configPath→.bak before manifest fetch. Restore on failure: automatic fallback from .bak on download/schema/hash failure. Boot integrity check: PRAGMA integrity_check runs on boot with [R]estore/[K]eep/[A]sk recovery prompt. Settings: Database Management sub-screen with version display, rollback, and delete-backup. Defense-in-depth: tryRestoreFromBackup() validates SQLite magic + schema + SHA-256 hash. 5 new doctest cases all passing. Build: zero warnings. |

### STEP-P3 — Document GitHub URL threat model (account compromise boundary)
| Field      | Value |
|------------|-------|
| Layer      | DOCUMENTATION |
| Priority   | LOW |
| Status     | [x] DONE |
| Files      | .ai/security.md |
| Goal       | Document the stability guarantees of raw.githubusercontent.com URLs and the bounded risk of GitHub account compromise under the manifest-based system (P1). Declare account-compromise data corruption as out-of-scope for additional crypto signing at this stage. |
| Depends    | none |
| Done when  | .ai/security.md contains a dedicated section documenting the GitHub URL threat model, explaining why no additional DB signing is required, and stating the out-of-scope declaration. |
| Completed  | **2026-06-08** — New "GitHub URL Threat Model" section (67 lines) added to .ai/security.md covering: raw.githubusercontent.com URL stability (immutable per-commit, mutable branch, CDN availability, rate limiting), account compromise risk table (3 attack vectors with impact/likelihood/mitigation), scope decision against additional crypto signing (4 reasons: sufficient existing protection, key management burden, account compromise scope, bounded blast radius), and out-of-scope declaration with revisit conditions. |

## Phase 2 — Core Tools UI
### STEP-12 — Implement tools entry screen and category browser
| Field      | Value |
|------------|-------|
| Layer      | L1+L2 |
| Priority   | HIGH |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Complete tools entry screen with browse/search/filter options and functional category browser |
| Depends    | STEP-P1 |
| Done when  | User can browse categories and view tools in each category from the tools entry screen |
| Completed  | **2026-06-09** — Tools entry screen category browser now reads from the canonical `categories` table (CategoryD) instead of extracting from ToolD (STEP-07 dependency). Added `CategoryDTO` to `svc_dto.h` and `getCategoryList()` to `SvcTools` service layer, sorted by `display_order` then name. Updated `showCategories()` in UI_tools.cpp to display categories as `name — description`. Removed now-unused `getCategories()` dead code and cleaned up includes (`<cctype>`, `<set>`). Added ambiguous-input feedback to `showToolsByCategory()` for consistent UX. Build: zero warnings across all 5 targets. Tests: 27/28 passing (1 pre-existing ConfigManager failure). Code review: APPROVED ✅. |

### STEP-13 — Complete tool detail screen (shared endpoint)
| Field      | Value |
|------------|-------|
| Layer      | L1+L2 |
| Priority   | HIGH |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement tool detail screen with description, flags, templates, and special options for metasploit/recon-ng |
| Depends    | STEP-12 |
| Done when  | Tool detail screen shows description, all flags, and templates for any selected tool |
| Completed  | **2026-06-09** — Tool detail screen implemented with full description, individual flags (name, description, protocols, root indicator), and templates (template_name, description, protocols, root/flag indicators). Added `getToolById()`, `getFlagsByToolId()`, `getTemplatesByToolId()` to SvcTools service layer with L0→DTO mappers for ToolFlag and Template. Created `svc_strings.h` as L1 re-export header to fix L2→L0 direct `strings.h` include (architectural layering). All user-facing strings in detail screen use `Strings::get()` from the string table. Build: zero warnings across all targets. Tests: 27/28 passing (1 pre-existing). Code review: APPROVED ✅. |

### STEP-14 — Implement template fill + placeholder prompting + save command
| Field      | Value |
|------------|-------|
| Layer      | L1+L2 |
| Priority   | HIGH |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L1-services/src/svc_tools.cpp, L0-core/src/UserDataManager.cpp |
| Goal       | Enable template placeholder filling, command preview, and saving to saved commands |
| Depends    | STEP-13 |
| Done when  | User can select a template, fill placeholders, preview command, and save it to saved commands |
| Completed  | **2026-06-09** — Template fill, placeholder prompting, and save command implemented. Added `buildCommand()` and `saveTemplateCommand()` to SvcTools service layer. Added interactive `showTemplateFill()` UI that prompts for target (IP/hostname) and optional port, displays a box-drawn command preview using `[sudo ]tool flags target [-p port]` format, and saves via `UserDataManager::saveCommand()`. Templates are numbered `[1]...[N]` in the tool detail screen for direct selection. Fixed argument injection vulnerability in `sanitizeInput()` (added space to rejection list). All navigation uses `isQuit()`/`isBack()` consistently. Build: zero warnings. Tests: 27/28 passing (1 pre-existing). Code review: APPROVED ✅. |

### STEP-15 — Implement metasploit vulnerabilities sub-menu
| Field      | Value |
|------------|-------|
| Layer      | L1+L2 |
| Priority   | HIGH |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Add vulnerabilities sub-menu to metasploit tool detail with search, filter, and show all options |
| Depends    | STEP-14 |
| Done when  | Metaspoit tool detail shows Vulnerabilities option and opens sub-menu when selected |
| Completed  | **2026-06-09** — Vulnerabilities sub-menu implemented with browse, filter (by severity/access/platform), search (by name/exact match), and "show all" options. Vulnerability detail screen shows name, description, severity, access, platform, service, metasploit path, and references. Code review identified 5 defects: Paginator lifecycle (F1-critical), name-based selection with ANSI codes (F2-high), incorrect LIKE documentation (F3-high), stale TODO (F4-medium), ::tolower portability (F6-medium), missing column validation (F7-low). All 6 defects fixed. Build: zero warnings. Tests: 27/28 passing. |

### STEP-16 — Implement recon-ng modules sub-menu
| Field      | Value |
|------------|-------|
| Layer      | L1+L2 |
| Priority   | HIGH |
| Status     | [x] DONE |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Add modules sub-menu to recon-ng tool detail with search, filter, and show all options |
| Depends    | STEP-15 |
| Done when  | Recon-ng tool detail shows Modules option and opens sub-menu when selected |
| Completed  | **2026-06-09** — Recon-ng modules sub-menu implemented with browse, filter (by type/platform), search (by name/exact match), and "show all" options. Module detail screen shows path, platform, type, API, mode (active/passive), loud, output, and description. Mirrors the vulnerability pattern from STEP-15. Code review: 1 high (tolower UB risk), 2 medium (missing cctype includes), 2 low (include ordering) — all fixed. Build: zero warnings. Tests: 27/28 passing. |

## Phase 3 — OPSEC + Network System
### STEP-17 — Implement Shadow Swap update system (L0 infrastructure) (STRATEGIC-4)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L0-core/include/db_cache_manager.h, L0-core/src/db_cache_manager.cpp, L0-core/include/config_manager.h, L0-core/src/config_manager.cpp |
| Goal       | Implement background download to tguide.db.tmp, atomic file swap on restart/exit, and internal update frequency tracking (Aggressive/Balanced/Passive/Stealth) |
| Depends    | STEP-16 |
| Done when  | Database updates download to tguide.db.tmp, swap occurs automatically on application restart/exit, and update frequency configuration is stored in config

### STEP-18 — Implement Shadow Swap update settings UI (L2) (STRATEGIC-4)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_settings.cpp, L2-Interface_Engine/src/UI_Engine.cpp, L2-Interface_Engine/includes/UI_utils.h |
| Goal       | Implement user notification "[ Update Ready - Restart to Apply ]" indicator and settings menu for update frequency (Aggressive/Balanced/Passive/Stealth) |
| Depends    | STEP-17 |
| Done when  | Settings screen shows update frequency option, UI displays "[ Update Ready - Restart to Apply ]" when swap is pending, and user can change update frequency in settings

### STEP-19 — Implement OPSEC connectivity modes (L0) (STRATEGIC-3)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L0-core/include/config_manager.h, L0-core/src/config_manager.cpp, L0-core/include/db_cache_manager.h, L0-core/src/db_cache_manager.cpp |
| Goal       | Implement OPSEC modes (Stealth/Balanced/Aggressive) that modify connection behavior, timeout values, and update check frequency |
| Depends    | STEP-18 |
| Done when  | Config manager stores OPSEC mode setting and db_cache_manager adjusts network behavior based on selected mode

### STEP-P2 — Implement DNS-Only Connectivity Check (deferred from proposal review)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | MEDIUM |
| Status     | [ ] VISION |
| Files      | L0-core/src/DBResolver.cpp, L0-core/include/DBResolver.h |
| Goal       | Replace any port-based/ping connectivity probing with getaddrinfo() checks against well-known DNS resolver hostnames (dns.google, one.one.one.one, resolver.opendns.com). DNS queries handled by system resolver; no packets reach external servers. Called only immediately before fetchManifest(). |
| Depends    | STEP-P1 |
| Done when  | DBResolver::hasInternetAccess() uses getaddrinfo() only; no ICMP/HTTP probes to external hosts; function called only before manifest fetch, not at startup or in background context. |

### STEP-20 — Implement randomized connection check (L0) (STRATEGIC-5)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L0-core/include/db_cache_manager.h, L0-core/src/db_cache_manager.cpp |
| Goal       | Implement randomized timing for database integrity checks to prevent pattern detection |
| Depends    | STEP-19 |
| Done when  | Database integrity checks occur at random intervals within configured bounds rather than fixed schedule

### STEP-21 — Implement search index cache clearing on shadow swap (L1)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp |
| Goal       | Clear search index when shadow swap occurs to prevent stale search results after database update |
| Depends    | STEP-20 |
| Done when  | svc_tools.cpp contains code that clears s_index during database swap operations

### STEP-22 — Implement enhanced search algorithm (L1)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement search with fuzzy matching, Levenshtein distance, and partial word matching for better user experience |
| Depends    | STEP-21 |
| Done when  | Search returns results for typos, partial names, and related terms using improved matching algorithms

## Phase 4 — User Data Screens
### STEP-23 — Implement saved commands service (L1)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_savedCommands.cpp, L1-services/includes/svc_savedCommands.h |
| Goal       | Implement service layer for CRUD operations on saved commands, bridging UI to UserDataManager persistence |
| Depends    | STEP-22 |
| Done when  | svc_savedCommands provides list, add, delete, and update functions that read/write via UserDataManager

### STEP-24 — Implement saved commands UI screen (L2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_savedCommands.cpp, L2-Interface_Engine/includes/UI_savedCommands.h |
| Goal       | Implement saved commands screen with list view, select to fill placeholders, delete, and execute options |
| Depends    | STEP-23 |
| Done when  | User can browse saved commands, select one to fill placeholders, delete commands, and see command preview

### STEP-25 — Implement saved scripts service (L1)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_savedScripts.cpp, L1-services/includes/svc_savedScripts.h |
| Goal       | Implement service layer for CRUD operations on saved scripts, bridging UI to UserDataManager persistence |
| Depends    | STEP-24 |
| Done when  | svc_savedScripts provides list, add, delete, and update functions that read/write via UserDataManager

### STEP-26 — Implement saved scripts UI screen (L2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_savedScripts.cpp, L2-Interface_Engine/includes/UI_savedScripts.h |
| Goal       | Implement saved scripts screen with list view, select to view, delete, and execute options |
| Depends    | STEP-25 |
| Done when  | User can browse saved scripts, select one to view details, delete scripts, and see script preview

### STEP-27 — Implement favorites storage in UserDataManager (L0) (NEW-2)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L0-core/include/UserDataManager.h, L0-core/src/UserDataManager.cpp |
| Goal       | Add favorites set/vector to UserDataManager with load/save logic in favorites.json |
| Depends    | STEP-26 |
| Done when  | UserDataManager loads and saves a favorites list from favorites.json with add/remove/check functions

### STEP-28 — Implement favorites service (L1) (NEW-2)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Add favorite toggle, list, and check functions to svc_tools service layer bridging UI to UserDataManager favorites |
| Depends    | STEP-27 |
| Done when  | svc_tools provides toggleFavorite(), isFavorite(), getFavorites() functions

### STEP-29 — Implement favorites UI toggle and menu (L2) (NEW-2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/includes/UI_tools.h, L2-Interface_Engine/src/UI_Engine.cpp |
| Goal       | Add favorite toggle (star icon) to tool detail screen and a Favorites section in main menu |
| Depends    | STEP-28 |
| Done when  | User can favorite/unfavorite from tool detail and see a Favorites section in main menu listing favorited tools

### STEP-30 — Implement recently viewed history in UserDataManager (L0) (NEW-3)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L0-core/include/UserDataManager.h, L0-core/src/UserDataManager.cpp |
| Goal       | Add recently viewed list (max 10) to UserDataManager with load/save logic in history.json |
| Depends    | STEP-29 |
| Done when  | UserDataManager tracks last 10 viewed tools with tool id and timestamp, persisted to history.json

### STEP-31 — Implement recently viewed history service (L1) (NEW-3)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Add view tracking functions to svc_tools: recordView(toolId) and getRecentViews() bridging UI to UserDataManager |
| Depends    | STEP-30 |
| Done when  | svc_tools records each tool view and provides the recent list sorted by most recent

### STEP-32 — Implement recently viewed bar in main menu (L2) (NEW-3)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_Engine.cpp |
| Goal       | Display a Recently Viewed bar in main menu showing last 5 viewed tools as quick-select options |
| Depends    | STEP-31 |
| Done when  | Main menu shows "Recently Viewed" section with up to 5 tool names that can be selected directly

## Phase 5 — Script Generator & Clipboard
### STEP-33 — Implement template selection and placeholder fill logic (L1)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_generator.cpp, L1-services/includes/svc_generator.h |
| Goal       | Implement multi-template selection, placeholder detection, and interactive fill logic for script generation |
| Depends    | STEP-32 |
| Done when  | svc_generator can select a template, detect all placeholders, prompt for values, and produce a filled command string

### STEP-34 — Implement generator wizard flow UI (L2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_generator.cpp, L2-Interface_Engine/includes/UI_generator.h |
| Goal       | Implement step-by-step wizard UI: template selection, placeholder prompting, and command preview |
| Depends    | STEP-33 |
| Done when  | User can navigate through generator wizard: select template, fill each placeholder, and preview command

### STEP-35 — Implement .sh file generation and write logic (L1)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_generator.cpp, L1-services/includes/svc_generator.h |
| Goal       | Implement file write logic for .sh script output: shebang, command body, and save to user scripts directory |
| Depends    | STEP-34 |
| Done when  | svc_generator can write a complete .sh file with proper shebang and command body to the saved scripts path

### STEP-36 — Implement script save confirmation screen (L2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_generator.cpp |
| Goal       | Implement name prompt, save confirmation with existing-name warning, and success notification after script write |
| Depends    | STEP-35 |
| Done when  | User can name the script, confirm overwrite if name exists, and see success confirmation after save

### STEP-37 — Implement live preview during build service (L1) (STRATEGIC-8)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_generator.cpp, L1-services/includes/svc_generator.h |
| Goal       | Implement live command preview generation: build command string incrementally as placeholders are filled |
| Depends    | STEP-36 |
| Done when  | svc_generator provides a getPreview() function that returns the current command string state during filling

### STEP-38 — Implement live preview pane in generator UI (L2) (STRATEGIC-8)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_generator.cpp |
| Goal       | Display a live preview pane that updates in real-time as the user fills placeholders in the generator wizard |
| Depends    | STEP-37 |
| Done when  | Generator wizard shows a live preview section that updates after each placeholder is filled

### STEP-39 — Implement copy to clipboard service (L1) (NEW-4)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_clipboard.cpp, L1-services/includes/svc_clipboard.h |
| Goal       | Implement cross-platform clipboard service with xclip (Linux), pbcopy (macOS), and clip.exe (Windows) fallback |
| Depends    | STEP-38 |
| Done when  | svc_clipboard provides copyToClipboard(text) that works on all target platforms with automatic detection

### STEP-40 — Implement copy to clipboard UI options (L2) (NEW-4)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/src/UI_generator.cpp, L2-Interface_Engine/src/UI_savedCommands.cpp |
| Goal       | Add [C] copy option to tool detail screen, generator preview, and saved commands browse screen |
| Depends    | STEP-39 |
| Done when  | User can press [C] to copy command/script text to clipboard from tool detail, generator, and saved commands screens

## Phase 6 — Community Extensions
### STEP-41 — Implement extensions folder scanning (L0) (STRATEGIC-2)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L0-core/include/path_resolver.h, L0-core/include/config_manager.h, L0-core/src/config_manager.cpp |
| Goal       | Add extensions directory path to PathResolver and implement file scanning for .json extension definitions |
| Depends    | STEP-40 |
| Done when  | Config manager provides an extensions path and a scan function that enumerates available extension files

### STEP-42 — Implement runtime extension merge with priority (L1) (STRATEGIC-2)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement runtime merge of extension data that overrides built-in database data when IDs conflict |
| Depends    | STEP-41 |
| Done when  | Extension tools appear in browse/search results and override built-in tools with the same ID

## Phase 7 — Filters
### STEP-43 — Implement flags filter service (L1) (FUTURE-2)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement filterByFlags() function that filters tools by specific flag presence in tool detail screen |
| Depends    | STEP-42 |
| Done when  | svc_tools provides a getFlagsForTool() and filterToolsByFlag() for flag-based filtering

### STEP-44 — Implement flags filter UI (L2) (FUTURE-2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/includes/UI_tools.h |
| Goal       | Add [C] flags filter option to tool detail screen showing available flags with selectable filter |
| Depends    | STEP-43 |
| Done when  | Tool detail screen shows [C] flags option and user can select a flag to filter tools by it

### STEP-45 — Implement templates filter service (L1) (FUTURE-3)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement filterByTemplate() function that filters tools that have specific template types |
| Depends    | STEP-44 |
| Done when  | svc_tools provides a getTemplatesForTool() and filterToolsByTemplate() for template-based filtering

### STEP-46 — Implement templates filter UI (L2) (FUTURE-3)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/includes/UI_tools.h |
| Goal       | Add [E] templates filter option to tool detail screen showing available templates with selectable filter |
| Depends    | STEP-45 |
| Done when  | Tool detail screen shows [E] templates option and user can select a template to filter tools by it

### STEP-47 — Implement vulnerability filter service (L1) (FUTURE-4)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement filterByVulnerability() function that filters tools related to specific vulnerability types |
| Depends    | STEP-46 |
| Done when  | svc_tools provides vulnerability-based filtering for tools in the browse/search results

### STEP-48 — Implement vulnerability filter UI (L2) (FUTURE-4)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/includes/UI_tools.h |
| Goal       | Add vulnerability filter option to tools browse screen with vulnerability type selection |
| Depends    | STEP-47 |
| Done when  | Browse screen includes vulnerability filter option and results update when vulnerability type is selected

### STEP-49 — Implement module filter service (L1) (FUTURE-5)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement filterByModule() function that filters tools related to specific modules |
| Depends    | STEP-48 |
| Done when  | svc_tools provides module-based filtering for tools in the browse/search results

### STEP-50 — Implement module filter UI (L2) (FUTURE-5)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/includes/UI_tools.h |
| Goal       | Add module filter option to tools browse screen with module type selection |
| Depends    | STEP-49 |
| Done when  | Browse screen includes module filter option and results update when module type is selected

## Phase 8 — New Features
### STEP-51 — Implement database stats service (L1) (NEW-5)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | LOW |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_stats.cpp, L1-services/includes/svc_stats.h |
| Goal       | Implement service to aggregate database statistics: total tools, per-category counts, last updated timestamp |
| Depends    | STEP-50 |
| Done when  | svc_stats provides getStats() returning tool count, category breakdown, and database metadata

### STEP-52 — Implement database stats UI screen (L2) (NEW-5)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | LOW |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_stats.cpp, L2-Interface_Engine/includes/UI_stats.h, L2-Interface_Engine/src/UI_Engine.cpp |
| Goal       | Implement stats screen accessible from main menu showing database statistics with formatted output |
| Depends    | STEP-51 |
| Done when  | Main menu has a Stats option that displays tool counts, category breakdown, and database info

### STEP-53 — Add Cloud Security tool modules to database (L0) (STRATEGIC-6)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | data_adder.cpp, L0-core/src/DatabaseManager.cpp, L0-core/include/DatabaseManager.h |
| Goal       | Add Cloud Security tool entries (AWS, GCP, Azure security tools) to database schema and seed data |
| Depends    | STEP-52 |
| Done when  | Database includes Cloud Security category with relevant tools, descriptions, flags, and templates

### STEP-54 — Add AI Hacking tool modules to database (L0) (STRATEGIC-7)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | data_adder.cpp, L0-core/src/DatabaseManager.cpp, L0-core/include/DatabaseManager.h |
| Goal       | Add AI Hacking tool entries (prompt injection, model security, adversarial ML tools) to database |
| Depends    | STEP-53 |
| Done when  | Database includes AI Hacking category with relevant tools, descriptions, flags, and templates

### STEP-55 — Implement run script from inside tguide service (L1) (FUTURE-6)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | LOW |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_generator.cpp, L1-services/includes/svc_generator.h |
| Goal       | Implement executeScript(path) function that runs a saved .sh script via subprocess and captures output |
| Depends    | STEP-54 |
| Done when  | svc_generator provides executeScript() that runs a script, captures stdout/stderr, and returns exit code

### STEP-56 — Implement run script UI (L2) (FUTURE-6)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | LOW |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_generator.cpp, L2-Interface_Engine/src/UI_savedScripts.cpp |
| Goal       | Add [R] run option to saved scripts screen and generator confirmation screen to execute scripts inline |
| Depends    | STEP-55 |
| Done when  | User can run a saved script from UI, see live output, and return to menu after execution completes

### STEP-57 — Implement script generator pre-loaded from tool detail service (L1) (FUTURE-7)
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | LOW |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/src/svc_generator.cpp, L1-services/includes/svc_generator.h |
| Goal       | Implement preloadTemplate(toolId) function that passes selected tool's template directly to generator |
| Depends    | STEP-56 |
| Done when  | svc_generator accepts a pre-selected template from svc_tools and initializes wizard with it

### STEP-58 — Implement script generator pre-loaded from tool detail UI (L2) (FUTURE-7)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | LOW |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/src/UI_generator.cpp |
| Goal       | Add [G] generate script option to tool detail screen that opens generator with the tool's template pre-loaded |
| Depends    | STEP-57 |
| Done when  | Tool detail screen shows [G] option and selecting it opens generator wizard with template already selected

## Phase 9 — Settings & Polish
### STEP-59 — Implement color toggle in settings screen (L2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_settings.cpp, L2-Interface_Engine/includes/UI_settings.h, L2-Interface_Engine/includes/UI_colors.h |
| Goal       | Add color enable/disable toggle to settings screen that persists to config and immediately updates UI |
| Depends    | STEP-58 |
| Done when  | Settings screen has color toggle, changing it persists to config, and UI updates colors immediately

### STEP-60 — Implement key bindings configuration in settings (L2)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | LOW |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_settings.cpp, L2-Interface_Engine/includes/UI_settings.h, L0-core/include/config_manager.h |
| Goal       | Add customizable key bindings for common actions (back, quit, copy, generate, run) in settings screen |
| Depends    | STEP-59 |
| Done when  | Settings screen shows key binding list, user can remap keys, and changes persist to config

### STEP-61 — Remove dev-only code before release
| Field      | Value |
|------------|-------|
| Layer      | CLEANUP |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | data_adder.cpp, L0-core/src/DatabaseManager.cpp, L0-core/include/DatabaseManager.h, CoreRunner.cpp |
| Goal       | Remove data_adder.cpp from build, remove BackupManager and add/del methods from DatabaseManager, and remove dev-only bootstrap code from CoreRunner |
| Depends    | STEP-60 |
| Done when  | data_adder.cpp is removed from CMakeLists.txt, BackupManager is removed, and no dev-only code remains in release build

## Phase 10 — Pre-Release & Packaging
### STEP-62 — Cross-platform testing and validation
| Field      | Value |
|------------|-------|
| Layer      | QA |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | All |
| Goal       | Test tguide on all target platforms (Linux, Termux, Windows, macOS) and fix platform-specific issues |
| Depends    | STEP-61 |
| Done when  | Tguide compiles and runs without errors on Linux, Termux, Windows, and macOS

### STEP-63 — Create AUR package for Arch Linux (FUTURE-8)
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | PKGBUILD (new), .SRCINFO (new) |
| Goal       | Create AUR PKGBUILD and .SRCINFO for Arch Linux distribution with proper dependencies and install paths |
| Depends    | STEP-62 |
| Done when  | Tguide is installable via `yay -S tguide` or similar AUR helper

### STEP-64 — Create Homebrew formula for macOS (FUTURE-8)
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | tguide.rb (new formula) |
| Goal       | Create Homebrew formula for macOS distribution with proper dependencies and install paths |
| Depends    | STEP-62 |
| Done when  | Tguide is installable via `brew install tguide`

### STEP-65 — Create Termux package (FUTURE-8)
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | build-package.sh (new), Termux build scripts |
| Goal       | Create Termux package build script with proper compilation flags for Android environment |
| Depends    | STEP-62 |
| Done when  | Tguide compiles and runs in Termux environment on Android

### STEP-66 — Create .deb package for Debian/Ubuntu (FUTURE-8)
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | MEDIUM |
| Status     | [ ] TODO |
| Files      | debian/ (new directory with control, rules, changelog, compat) |
| Goal       | Create .deb packaging structure for Debian-based Linux distributions |
| Depends    | STEP-62 |
| Done when  | `dpkg-buildpackage` produces a working .deb package

## Future Vision
### STRATEGIC-1 — AI Assistant (llama.cpp integration)
| Field      | Value |
|------------|-------|
| Layer      | Future Vision |
| Priority   | LOW |
| Status     | [ ] VISION |
| Goal       | Integrate llama.cpp for local AI-powered assistance: natural language querying of tools, intelligent command suggestions, and contextual help |
| Dependencies | llama.cpp library, significant refactoring for async I/O |
| Rationale   | AI assistant requires architectural changes beyond current scope and would benefit from having a stable, feature-complete application first |

## Step Index Table
| Step | Phase | Layer | Goal |
|------|-------|-------|------|
| STEP-00 | Phase 0 | BUGFIX | Fix root check in CoreRunner.cpp |
| STEP-01 | Phase 0 | BUGFIX | Fix direct cin >> usage in UI_Engine.cpp |
| STEP-02 | Phase 0 | BUGFIX | Fix stoi crash risk in toNumber() |
| STEP-03 | Phase 0 | BUGFIX | Fix search index memory leak |
| STEP-04 | Phase 0 | BUGFIX | Fix command injection risk in script generation |
| STEP-05 | Phase 0 | BUGFIX | Fix isatty() missing for non-interactive shells |
| STEP-A1 | Phase 0 | L2 (Architecture) | Fix A1 architectural violation (UI→L0 direct includes) |
| STEP-A2 | Phase 0 | L0 (Architecture) | Fix A2 architectural violation (extern callbacks → ErrorHandler) |
| STEP-A3 | Phase 0 | L0+L1+L2 (Architecture) | Fix A3 architectural violation (Global Mutable State) |
| STEP-A7 | Phase 0 | CMake (Architecture) | Fix A7 architectural violation (No Build-Time Layer Enforcement) |
| STEP-A4 | Phase 0 | L2 (Architecture) | Fix A4 architectural violation (Disclaimer bypasses L1) |
| STEP-A6 | Phase 0 | L1 (Architecture) | Fix A6 architectural violation (No DTOs in L1) |
| STEP-A8 | Phase 0 | BOOTSTRAP (Architecture) | Fix A8 architectural violation (ErrorHandler timing hole) |
| STEP-A9 | Phase 0 | BOOTSTRAP (Architecture) | Fix A9 architectural violation (CoreRunner over-instantiation) |
| STEP-A03 | Phase 0 | L0 (Security) | Fix libcurl SSL/TLS verification in DBResolver.cpp (A-03) |
| STEP-CP1 | Phase 0 | CROSS-PLATFORM | Implement macOS cross-platform portability fixes |
| STEP-06 | Phase 1 | L0+L1+L2 | Fix error handling contract implementation |
| STEP-07 | Phase 1 | L0 | Update database schema for short_desc and categories table (DONE) |
| STEP-08 | Phase 1 | L0 | Implement UserDataManager for saved commands and scripts |
| STEP-09 | Phase 1 | L0 | Add I18n / Localization Framework foundation (NEW-1) |
| STEP-10 | Phase 1 | L0 | Fix path resolver for Linux user-space only (DONE) |
| STEP-P1 | Phase 1 | L0 | Implement Dynamic Version Manifest (replaces STEP-11) (DONE) |
| STEP-P4 | Phase 1 | L0 + L2 | Implement Database Rollback & Recovery System |
| STEP-P3 | Phase 1 | DOCUMENTATION | Document GitHub URL threat model (account compromise) |
| STEP-12 | Phase 2 | L1+L2 | Implement tools entry screen and category browser |
| STEP-13 | Phase 2 | L1+L2 | Complete tool detail screen (shared endpoint) |
| STEP-14 | Phase 2 | L1+L2 | Implement template fill + placeholder prompting + save command |
| STEP-15 | Phase 2 | L1+L2 | Implement metasploit vulnerabilities sub-menu |
| STEP-16 | Phase 2 | L1+L2 | Implement recon-ng modules sub-menu |
| STEP-17 | Phase 3 | L0 | Implement Shadow Swap update system infrastructure (STRATEGIC-4) |
| STEP-18 | Phase 3 | L2 | Implement Shadow Swap update settings UI (STRATEGIC-4) |
| STEP-19 | Phase 3 | L0 | Implement OPSEC connectivity modes (STRATEGIC-3) |
| STEP-P2 | Phase 3 | L0 | Implement DNS-Only Connectivity Check (deferred) |
| STEP-20 | Phase 3 | L0 | Implement randomized connection check (STRATEGIC-5) |
| STEP-21 | Phase 3 | L1 | Implement search index cache clearing on shadow swap |
| STEP-22 | Phase 3 | L1 | Implement enhanced search algorithm |
| STEP-23 | Phase 4 | L1 | Implement saved commands service |
| STEP-24 | Phase 4 | L2 | Implement saved commands UI screen |
| STEP-25 | Phase 4 | L1 | Implement saved scripts service |
| STEP-26 | Phase 4 | L2 | Implement saved scripts UI screen |
| STEP-27 | Phase 4 | L0 | Implement favorites storage in UserDataManager (NEW-2) |
| STEP-28 | Phase 4 | L1 | Implement favorites service (NEW-2) |
| STEP-29 | Phase 4 | L2 | Implement favorites UI toggle and menu (NEW-2) |
| STEP-30 | Phase 4 | L0 | Implement recently viewed history in UserDataManager (NEW-3) |
| STEP-31 | Phase 4 | L1 | Implement recently viewed history service (NEW-3) |
| STEP-32 | Phase 4 | L2 | Implement recently viewed bar in main menu (NEW-3) |
| STEP-33 | Phase 5 | L1 | Implement template selection and placeholder fill logic |
| STEP-34 | Phase 5 | L2 | Implement generator wizard flow UI |
| STEP-35 | Phase 5 | L1 | Implement .sh file generation and write logic |
| STEP-36 | Phase 5 | L2 | Implement script save confirmation screen |
| STEP-37 | Phase 5 | L1 | Implement live preview during build service (STRATEGIC-8) |
| STEP-38 | Phase 5 | L2 | Implement live preview pane in generator UI (STRATEGIC-8) |
| STEP-39 | Phase 5 | L1 | Implement copy to clipboard service (NEW-4) |
| STEP-40 | Phase 5 | L2 | Implement copy to clipboard UI options (NEW-4) |
| STEP-41 | Phase 6 | L0 | Implement extensions folder scanning (STRATEGIC-2) |
| STEP-42 | Phase 6 | L1 | Implement runtime extension merge with priority (STRATEGIC-2) |
| STEP-43 | Phase 7 | L1 | Implement flags filter service (FUTURE-2) |
| STEP-44 | Phase 7 | L2 | Implement flags filter UI (FUTURE-2) |
| STEP-45 | Phase 7 | L1 | Implement templates filter service (FUTURE-3) |
| STEP-46 | Phase 7 | L2 | Implement templates filter UI (FUTURE-3) |
| STEP-47 | Phase 7 | L1 | Implement vulnerability filter service (FUTURE-4) |
| STEP-48 | Phase 7 | L2 | Implement vulnerability filter UI (FUTURE-4) |
| STEP-49 | Phase 7 | L1 | Implement module filter service (FUTURE-5) |
| STEP-50 | Phase 7 | L2 | Implement module filter UI (FUTURE-5) |
| STEP-51 | Phase 8 | L1 | Implement database stats service (NEW-5) |
| STEP-52 | Phase 8 | L2 | Implement database stats UI screen (NEW-5) |
| STEP-53 | Phase 8 | L0 | Add Cloud Security tool modules to database (STRATEGIC-6) |
| STEP-54 | Phase 8 | L0 | Add AI Hacking tool modules to database (STRATEGIC-7) |
| STEP-55 | Phase 8 | L1 | Implement run script from inside tguide service (FUTURE-6) |
| STEP-56 | Phase 8 | L2 | Implement run script UI (FUTURE-6) |
| STEP-57 | Phase 8 | L1 | Implement script generator pre-loaded from tool detail service (FUTURE-7) |
| STEP-58 | Phase 8 | L2 | Implement script generator pre-loaded from tool detail UI (FUTURE-7) |
| STEP-59 | Phase 9 | L2 | Implement color toggle in settings screen |
| STEP-60 | Phase 9 | L2 | Implement key bindings configuration in settings |
| STEP-61 | Phase 9 | CLEANUP | Remove dev-only code before release |
| STEP-62 | Phase 10 | QA | Cross-platform testing and validation |
| STEP-63 | Phase 10 | PACKAGING | Create AUR package for Arch Linux (FUTURE-8) |
| STEP-64 | Phase 10 | PACKAGING | Create Homebrew formula for macOS (FUTURE-8) |
| STEP-65 | Phase 10 | PACKAGING | Create Termux package (FUTURE-8) |
| STEP-66 | Phase 10 | PACKAGING | Create .deb package for Debian/Ubuntu (FUTURE-8) |