# Tguide — Development Roadmap
## Version: 1.0.0
## Date: 2026-06-09
## Author: voidoxin

## Current State (from code scan)
- **v1.0 Focus**: This roadmap is now focused on delivering a stable v1.0.0 release. All Phase 0 (critical bug fixes), Phase 1 (foundation), and Phase 2 (core tools UI) steps are complete. The remaining work is organized into release-focused phases targeting database tooling, cross-platform bootstrap, core feature completion, database lifecycle, and packaging.
- **Phase 0 complete**: All 17 steps (STEP-00 through STEP-A06) including critical bug fixes, architectural violations, security hardening, cross-platform fixes, and test infrastructure.
- **Phase 1 complete**: All 7 steps (STEP-06 through STEP-P3) including error handling, database schema, UserDataManager, localization, path resolver, dynamic manifest, DB rollback/recovery, and threat model documentation.
- **Phase 2 complete**: All 5 steps (STEP-12 through STEP-16) including tools entry screen, tool detail screen, template fill/placeholder prompting, metasploit vulnerabilities, and recon-ng modules.
- **CoreRunner.cpp**: Bootstrap sequence fully implemented, root check eliminated, ErrorHandler registered early (A8 fix).
- **L0-core**: DatabaseManager has full CRUD, UserDataManager singleton integrated, path_resolver supports Linux/macOS, DBResolver with dynamic manifest + SSL/TLS hardening, DBCacheManager with backup/rollback, A1/A2/A3/A8/A9 architectural violations fixed.
- **L1-services**: svc_tools with search index, svc_generator with input sanitization, svc_dto decoupling layer, string table re-export, A6 DTO pattern fixed.
- **L2-Interface_Engine**: UI_Engine with readInput(), colors with isatty()/g_colorEnabled, tool detail/category/vulnerability/module screens, template fill workflow, generator wizard, A4 header isolation fixed, A7 build-time layer enforcement fixed.
- **Testing**: doctest framework with 28+ test cases across SHA256, ConfigManager, UserDataManager, DB recovery. CTest integration.
- **Key remaining work before v1.0**: Replace data_adder.cpp with professional Python toolchain (STEP-DB), fix database bootstrap for no-internet first boot (STEP-B1), cross-platform path resolution (STEP-B2), Windows support (STEP-B3), cross-platform validation (STEP-CP2), enhanced search (STEP-R1), saved commands/scripts screens (STEP-R2/R3), settings screen completion (STEP-R4), remove all stubs (STEP-R5), shadow swap update system (STEP-17/18), fix manifest URL to use GitHub Releases (STEP-MU), remove dev-only code (STEP-61), full QA (STEP-62), and packaging for AUR/Homebrew/Deb/Windows + v1.0 release (STEP-PK1-5).

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

## Release Phase R0 — Database Builder ⭐ HIGHEST PRIORITY
### STEP-DB — Professional Python database builder (replaces data_adder.cpp)
| Field      | Value |
|------------|-------|
| Layer      | TOOLING |
| Priority   | HIGHEST |
| Status     | [x] DONE |
| Files      | tools/data_adder.py (NEW), tools/build_db.py (NEW), tools/data/ (NEW directory), data_adder.cpp (DELETE) |
| Goal       | Replace data_adder.cpp with a Python CLI tool that add/list/delete database records interactively or via CLI commands, plus a YAML/JSON batch builder for producing signed seed databases with release manifests |
| Depends    | none |
| Done when  | `python tools/data_adder.py` can add/list/delete records in an existing DB; `python tools/build_db.py` can produce a valid seed DB from YAML; data_adder.cpp is deleted; manifest is auto-generated; schema validates |
| Completed  | **2026-06-09** — Two tools replace `data_adder.cpp`: `tools/data_adder.py` (interactive menu + CLI commands for add/list/delete/manifest on existing DB) and `tools/build_db.py` (YAML/JSON→DB batch builder with init/build/validate/manifest/dump). Seed data in `tools/data/` with 8 categories, 8 tools, 31 flags, 18 templates, 5 vulnerabilities, 13 options, 6 modules in both .yaml and .json. Code review: 8+7=15 issues found and fixed. `data_adder.cpp` deleted from repo and CMakeLists.txt build. **Post-completion enhancement** (commit 3fdf114): Added `reset` command — `cmd_reset()` archives current DB to `old_data/<dbname>_<timestamp>.db`, creates fresh empty DB with all 7 tables via safe temp-file-first approach (no data-loss window). Accessible via interactive menu option 22 (`interactive_reset()`) or CLI `python3 tools/data_adder.py reset [--force/-f]`. Module imports: `datetime`, `shutil`, `tempfile`, `schema`. **Post-completion enhancement** (commit a191a1f): Added `translations` table (8th table) with UNIQUE(table_name, row_id, column_name, lang) constraint and NOT NULL on all columns — schema.py `extra` field support enables inline UNIQUE clauses. `db_builder.py`: `_load_and_validate_translations()` with column-name whitelist via `TRANSLATABLE_COLUMNS` dict, translation insertion in build pipeline. `data_adder.py`: Full CRUD — `cmd_add_translation()` (UPSERT preserving existing ID), `cmd_list_translations()` (filterable by table/lang), `cmd_delete_translation()` — interactive menu options 23/24/25 for Add/View/Delete translations, CLI subcommand `translation {add,list,delete}`. Seed data: `tools/data/translations.yaml` and `.json` with 20 French translations covering all 8 category names+descriptions and 3 tool short_desc/descriptions (nmap, sqlmap, metasploit). |

## Release Phase R1 — Bootstrap & Cross-Platform Foundation
### STEP-B1 — Fix database bootstrap (no internet on first boot)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | L0-core/src/DBResolver.cpp, L0-core/include/DBResolver.h, CoreRunner.cpp, CMakeLists.txt |
| Goal       | Remove first-boot internet requirement: bundle a seed DB with the binary, detect fresh install, copy seed DB to user data dir, make manifest fetch non-fatal (graceful degradation if offline) |
| Depends    | none |
| Done when  | First boot succeeds without network; seed DB is bundled at `/usr/share/tguide/tguide.db` (Linux), `~/Library/Application Support/tguide/tguide.db` (macOS), `%APPDATA%/tguide/tguide.db` (Windows); manifest failure shows warning but does not exit |

### STEP-B2 — Cross-platform path resolution
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | L0-core/include/path_resolver.h, L0-core/src/DBResolver.cpp, CoreRunner.cpp |
| Goal       | Unify path resolution across all target platforms: Linux (XDG), macOS (Application Support), Windows (APPDATA/LOCALAPPDATA), Termux (~/../usr/share). Add installDbFile() returning per-OS seed DB path. |
| Depends    | STEP-B1 |
| Done when  | path_resolver.h returns correct platform-specific paths for all target platforms; copyDefaultToConfig() uses installDbFile() instead of hardcoded dev-time path |

### STEP-B3 — Windows support
| Field      | Value |
|------------|-------|
| Layer      | CROSS-PLATFORM |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | CMakeLists.txt, L2-Interface_Engine/includes/UI_colors.h, CoreRunner.cpp |
| Goal       | Add Windows build support: enable ANSI colors via Windows API, handle SIGINT via SetConsoleCtrlHandler, add MSVC/clang-cl CMake configuration, fix POSIX-specific code paths |
| Depends    | STEP-B2 |
| Done when  | tguide compiles and runs on Windows without errors; colors work in Windows Terminal; Ctrl+C is handled gracefully |

### STEP-CP2 — Cross-platform validation testing
| Field      | Value |
|------------|-------|
| Layer      | QA |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | All |
| Goal       | Test tguide on all target platforms: Kali Linux, Ubuntu, Fedora, Arch Linux, macOS, Windows, Termux. Fix all platform-specific issues found. |
| Depends    | STEP-B3 |
| Done when  | tguide compiles, installs, and runs correctly on all 7 target platforms |

## Release Phase R2 — Core Feature Completion
### STEP-R1 — Implement enhanced search algorithm
| Field      | Value |
|------------|-------|
| Layer      | L1 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_tools.cpp, L1-services/includes/svc_tools.h |
| Goal       | Implement search with fuzzy matching, partial word matching, and case-insensitive search for better user experience (was STEP-22) |
| Depends    | STEP-CP2 |
| Done when  | Search returns results for typos, partial names, and related terms |

### STEP-R2 — Implement saved commands screen (was STEP-23 + STEP-24 merged)
| Field      | Value |
|------------|-------|
| Layer      | L1+L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_savedCommands.cpp (NEW), L1-services/includes/svc_savedCommands.h (NEW), L2-Interface_Engine/src/UI_savedCommands.cpp, L2-Interface_Engine/includes/UI_savedCommands.h |
| Goal       | Implement one complete saved commands feature: service layer (CRUD via UserDataManager) + UI screen (list, select, fill placeholders, delete, execute) |
| Depends    | STEP-R1 |
| Done when  | User can browse saved commands, select one to fill placeholders, delete, and see command preview |

### STEP-R3 — Implement saved scripts screen (was STEP-25 + STEP-26 merged)
| Field      | Value |
|------------|-------|
| Layer      | L1+L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L1-services/src/svc_savedScripts.cpp (NEW), L1-services/includes/svc_savedScripts.h (NEW), L2-Interface_Engine/src/UI_savedScripts.cpp, L2-Interface_Engine/includes/UI_savedScripts.h |
| Goal       | Implement one complete saved scripts feature: service layer (CRUD via UserDataManager) + UI screen (list, view, delete, execute) |
| Depends    | STEP-R2 |
| Done when  | User can browse saved scripts, select one to view details, delete scripts, and see script preview |

### STEP-R4 — Complete settings screen (color toggle + DB management)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_settings.cpp, L2-Interface_Engine/includes/UI_settings.h |
| Goal       | Complete the settings screen with: color enable/disable toggle (persisted to config), database version display, manual DB update trigger, backup restore option (from STEP-P4), and DB info display |
| Depends    | STEP-R3 |
| Done when  | Settings screen has all 5 features working; color toggle persists and takes effect immediately |

### STEP-R5 — Remove all "coming soon" stubs from codebase
| Field      | Value |
|------------|-------|
| Layer      | CLEANUP |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_tools.cpp, L2-Interface_Engine/src/UI_Engine.cpp, L2-Interface_Engine/includes/UI_tools.h, L1-services/src/svc_tools.cpp |
| Goal       | Remove or implement all "coming soon" / placeholder menu entries. If a feature isn't ready for v1.0, its menu entry must be hidden behind a compile-time flag or removed entirely. Audit for 183 stub/TODO references found in codebase. |
| Depends    | STEP-R4 |
| Done when  | No "coming soon", "TODO", "stub", or placeholder text remains in user-visible UI; dead code paths are removed |

## Release Phase R3 — Database Lifecycle
### STEP-17 — Shadow Swap update system (was Phase 3 STEP-17)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L0-core/src/DBResolver.cpp, L0-core/include/DBResolver.h, L0-core/include/db_cache_manager.h, L0-core/src/db_cache_manager.cpp |
| Goal       | Implement background download to tguide.db.tmp, atomic file swap on restart/exit, and update notification |
| Depends    | STEP-R5 |
| Done when  | Database updates download to .tmp file, binary swap occurs on next restart, update notification is stored in DBCacheManager |

### STEP-18 — Shadow Swap update UI (was Phase 3 STEP-18)
| Field      | Value |
|------------|-------|
| Layer      | L2 |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | L2-Interface_Engine/src/UI_settings.cpp, L2-Interface_Engine/src/UI_Engine.cpp |
| Goal       | Show "[ Update Ready — Restart to Apply ]" notification in main menu when shadow swap is pending; add update trigger in Settings |
| Depends    | STEP-17 |
| Done when  | UI shows update notification when swap is pending; Settings has "Check for Updates" and "Apply Update" options |

### STEP-MU — Fix manifest URL to use GitHub Releases (NEW)
| Field      | Value |
|------------|-------|
| Layer      | L0 |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | L0-core/src/DBResolver.cpp, .ai/security.md |
| Goal       | Change manifest URL from raw.githubusercontent.com/main branch to a permanent GitHub Releases URL so the released binary forever points to a stable manifest location independent of repo changes |
| Depends    | STEP-18 |
| Done when  | Manifest URL points to `https://github.com/voidoxin/Tguide/releases/latest/download/signed_manifest.json`; `main` branch URL is no longer in codebase |

## Release Phase R4 — Pre-Release & Packaging
### STEP-61 — Remove dev-only code before release (was Phase 9 STEP-61)
| Field      | Value |
|------------|-------|
| Layer      | CLEANUP |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | data_adder.cpp, CMakeLists.txt, CoreRunner.cpp, L0-core/src/DatabaseManager.cpp |
| Goal       | Remove data_adder.cpp from build system, remove any remaining dev-only bootstrap code from CoreRunner, add release build type configuration |
| Depends    | STEP-MU |
| Done when  | data_adder.cpp is removed from CMakeLists.txt; `cmake --build build --config Release` produces a clean build with no dev code |

### STEP-62 — Full QA testing (was Phase 10 STEP-62)
| Field      | Value |
|------------|-------|
| Layer      | QA |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | All |
| Goal       | Full regression test suite: all existing doctest tests pass, manual smoke test on all supported platforms, bootstrap/update/rollback scenarios verified, edge cases documented |
| Depends    | STEP-61 |
| Done when  | All tests pass on all platforms; QA report generated; no known P0/P1 bugs remain |

### STEP-PK1 — AUR package for Arch Linux
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | PKGBUILD (NEW), .SRCINFO (NEW) |
| Goal       | Create AUR PKGBUILD with proper dependencies (libcurl, sqlite), install paths (/usr/share/tguide/tguide.db), and release build |
| Depends    | STEP-62 |
| Done when  | `yay -S tguide` installs and runs correctly on Arch Linux |

### STEP-PK2 — Homebrew formula for macOS
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | Formula/tguide.rb (NEW) |
| Goal       | Create Homebrew formula with proper dependencies, macOS path support, and install targets |
| Depends    | STEP-62 |
| Done when  | `brew install tguide` installs and runs correctly on macOS |

### STEP-PK3 — .deb package for Debian/Kali/Ubuntu
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | debian/ (NEW directory: control, rules, changelog, compat, install) |
| Goal       | Create .deb packaging with proper dependencies, seed DB bundling, and system-wide install paths |
| Depends    | STEP-62 |
| Done when  | `dpkg-buildpackage` produces a working .deb; `apt install ./tguide.deb` works on Debian/Kali/Ubuntu |

### STEP-PK4 — Windows installer (ZIP/NSIS)
| Field      | Value |
|------------|-------|
| Layer      | PACKAGING |
| Priority   | HIGH |
| Status     | [ ] TODO |
| Files      | build/windows/installer.nsi (NEW), CMakeLists.txt |
| Goal       | Create Windows ZIP archive and NSIS installer with bundled DLLs and seed DB |
| Depends    | STEP-62 |
| Done when  | Windows installer produces working tguide.exe with colors, paths, and seed DB |

### STEP-PK5 — Release v1.0.0
| Field      | Value |
|------------|-------|
| Layer      | RELEASE |
| Priority   | CRITICAL |
| Status     | [ ] TODO |
| Files      | GitHub Releases, CHANGELOG.md (NEW) |
| Goal       | Tag v1.0.0, create GitHub Release with all artifacts (Linux binary, .deb, macOS Homebrew, Windows ZIP, AUR commit), write changelog, announce |
| Depends    | STEP-PK1, STEP-PK2, STEP-PK3, STEP-PK4 |
| Done when  | GitHub Release v1.0.0 is published with all platform artifacts; CHANGELOG.md documents all v1.0 features and changes |

## Future — v2.0 (Post-Release)

These features are explicitly cut from v1.0 scope and moved to a future v2.0 release:

- **OPSEC modes** (Stealth/Balanced/Aggressive connectivity) — was STEP-19, STEP-20
- **DNS-Only Connectivity Check** — was STEP-P2
- **Randomized timing** — was STEP-20
- **Favorites system** — was STEP-27, STEP-28, STEP-29
- **Recently viewed history** — was STEP-30, STEP-31, STEP-32
- **Script generator wizard** (beyond basic template fill from STEP-14) — was Phase 5
- **Clipboard integration** — was STEP-39, STEP-40
- **Extension system** — was Phase 6
- **Filters system** (flags/templates/vulnerability/module filters) — was Phase 7
- **Database stats screen** — was STEP-51, STEP-52
- **Cloud Security tools** — was STEP-53
- **AI Hacking tools** — was STEP-54
- **Run script inside tguide** — was STEP-55, STEP-56
- **Generator pre-loaded from tool detail** — was STEP-57, STEP-58
- **Key bindings configuration** — was STEP-60
- **Termux package** — was STEP-65
- **AI Assistant (llama.cpp)** — STRATEGIC-1
- **VISION items** (STEP-P2)

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
| STEP-A06 | Phase 0 | TEST | Add minimum viable test infrastructure with doctest |
| STEP-06 | Phase 1 | L0+L1+L2 | Fix error handling contract implementation |
| STEP-07 | Phase 1 | L0 | Update database schema for short_desc and categories table |
| STEP-08 | Phase 1 | L0 | Implement UserDataManager for saved commands and scripts |
| STEP-09 | Phase 1 | L0 | Add I18n / Localization Framework foundation (NEW-1) |
| STEP-10 | Phase 1 | L0 | Fix path resolver for Linux user-space only (no root required) |
| STEP-P1 | Phase 1 | L0 | Implement Dynamic Version Manifest (replaces STEP-11) |
| STEP-P4 | Phase 1 | L0 + L2 | Implement Database Rollback & Recovery System |
| STEP-P3 | Phase 1 | DOCUMENTATION | Document GitHub URL threat model (account compromise) |
| STEP-12 | Phase 2 | L1+L2 | Implement tools entry screen and category browser |
| STEP-13 | Phase 2 | L1+L2 | Complete tool detail screen (shared endpoint) |
| STEP-14 | Phase 2 | L1+L2 | Implement template fill + placeholder prompting + save command |
| STEP-15 | Phase 2 | L1+L2 | Implement metasploit vulnerabilities sub-menu |
| STEP-16 | Phase 2 | L1+L2 | Implement recon-ng modules sub-menu |
| STEP-DB | Release R0 | TOOLING | Professional Python database builder (replaces data_adder.cpp) |
| STEP-B1 | Release R1 | L0 | Fix database bootstrap (no internet on first boot) |
| STEP-B2 | Release R1 | L0 | Cross-platform path resolution |
| STEP-B3 | Release R1 | CROSS-PLATFORM | Windows support |
| STEP-CP2 | Release R1 | QA | Cross-platform validation testing |
| STEP-R1 | Release R2 | L1 | Implement enhanced search algorithm |
| STEP-R2 | Release R2 | L1+L2 | Implement saved commands screen (was STEP-23+24) |
| STEP-R3 | Release R2 | L1+L2 | Implement saved scripts screen (was STEP-25+26) |
| STEP-R4 | Release R2 | L2 | Complete settings screen (color toggle + DB management) |
| STEP-R5 | Release R2 | CLEANUP | Remove all "coming soon" stubs from codebase |
| STEP-17 | Release R3 | L0 | Shadow Swap update system (was Phase 3 STEP-17) |
| STEP-18 | Release R3 | L2 | Shadow Swap update UI (was Phase 3 STEP-18) |
| STEP-MU | Release R3 | L0 | Fix manifest URL to use GitHub Releases |
| STEP-61 | Release R4 | CLEANUP | Remove dev-only code before release (was Phase 9 STEP-61) |
| STEP-62 | Release R4 | QA | Full QA testing (was Phase 10 STEP-62) |
| STEP-PK1 | Release R4 | PACKAGING | AUR package for Arch Linux |
| STEP-PK2 | Release R4 | PACKAGING | Homebrew formula for macOS |
| STEP-PK3 | Release R4 | PACKAGING | .deb package for Debian/Kali/Ubuntu |
| STEP-PK4 | Release R4 | PACKAGING | Windows installer (ZIP/NSIS) |
| STEP-PK5 | Release R4 | RELEASE | Release v1.0.0 |
