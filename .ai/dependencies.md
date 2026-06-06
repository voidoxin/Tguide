# Tguide — Code Dependencies

> **Generated:** 2026-06-06  
> **Source:** Full codebase exploration via explorer agent(s), CMakeLists.txt analysis, include graph scanning  
> **Format:** `[name][fonctions used][files depende on this dependencie][code snippet][cause]`

---

## 1. SQLite3 (Bundled Amalgamation)

```
[name]
SQLite3 (embedded database engine)

[fonctions used]
sqlite3_open()          — open a database file connection
sqlite3_close()         — close a database connection
sqlite3_prepare_v2()    — compile a SQL statement into bytecode
sqlite3_step()          — execute one step of a prepared statement
sqlite3_finalize()      — destroy a prepared statement
sqlite3_exec()          — execute one or more SQL statements directly
sqlite3_bind_text()     — bind a TEXT value to a parameterized SQL statement
sqlite3_bind_int()      — bind an INTEGER value to a parameterized SQL statement
sqlite3_column_text()   — retrieve a TEXT column value from a result row
sqlite3_column_int()    — retrieve an INTEGER column value from a result row
sqlite3_errmsg()        — retrieve the last error message from a database handle
sqlite3*                — opaque database connection handle (type)
sqlite3_stmt*           — opaque prepared statement handle (type)
SQLITE_OK, SQLITE_ERROR, SQLITE_ROW, SQLITE_DONE, SQLITE_CORRUPT, SQLITE_TRANSIENT,
SQLITE_ABORT, SQLITE_BUSY  — result code constants

[files depende on this dependencie]
L0-core/include/DatabaseManager.h
L0-core/src/DatabaseManager.cpp
L0-core/src/DBResolver.cpp

[code snippet]
class DBSession {
    sqlite3* db;
public:
    DBSession(const std::string& path) : db(nullptr) {
        if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
            db = nullptr;
        } else {
            sqlite3_exec(db, "PRAGMA foreign_keys = ON;", 0, 0, nullptr);
        }
    }
    ~DBSession() { if (db) sqlite3_close(db); }
    bool execute(const char* sql, ...) {
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
            return false;
        // ... bind, step, finalize ...
        bool result = sqlite3_step(stmt) == SQLITE_DONE;
        sqlite3_finalize(stmt);
        return result;
    }
};

[cause]
SQLite3 provides the embedded relational database engine that powers the
entire tool reference system. All tool metadata (categories, tools, flags,
templates, vulnerabilities, modules) is stored in a single local SQLite
file (tguide.db). The bundled amalgamation (sqlite3.c + sqlite3.h) avoids
a system-package dependency and guarantees a known version (3.50.4) with
consistent behavior across all target platforms.
```

---

## 2. libcurl (System Library)

```
[name]
libcurl (client-side URL transfer library)

[fonctions used]
curl_easy_init()        — create a CURL easy session handle
curl_easy_setopt()      — configure transfer options on a CURL handle
curl_easy_perform()     — execute the configured transfer synchronously
curl_easy_cleanup()     — destroy a CURL easy session handle
CURL*                   — opaque easy session handle (type)
CURLcode                — return/error code enum
CURLoption              — option identifier enum (used with curl_easy_setopt)
CURLE_OK                — success return code
CURLE_SSL_CONNECT_ERROR / CURLE_PEER_FAILED_VERIFICATION / CURLE_SSL_CERTPROBLEM
                        — SSL-specific error codes
CURL_SSLVERSION_TLSv1_2 — minimum TLS version constant
CURLPROTO_HTTPS         — HTTPS protocol bitmask constant
LIBCURL_VERSION_NUM     — compile-time version check macro

curl_easy_setopt options used:
  CURLOPT_URL                 — target URL string
  CURLOPT_FOLLOWLOCATION      — follow HTTP redirects (1L)
  CURLOPT_MAXREDIRS           — maximum redirect chain length (5L)
  CURLOPT_WRITEFUNCTION       — custom write callback
  CURLOPT_WRITEDATA           — FILE* destination stream
  CURLOPT_FAILONERROR         — fail on HTTP 4xx/5xx (1L)
  CURLOPT_TIMEOUT             — total transfer timeout (30s)
  CURLOPT_CONNECTTIMEOUT      — connection establishment timeout (10s)
  CURLOPT_MAXFILESIZE         — maximum accepted file size (20MB)
  CURLOPT_SSL_VERIFYPEER      — verify server certificate (1L)
  CURLOPT_SSL_VERIFYHOST      — verify hostname matches certificate (2L)
  CURLOPT_SSLVERSION          — minimum TLS version (TLSv1.2)
  CURLOPT_PROTOCOLS_STR       — allowed protocols ("https") [>=7.85.0]
  CURLOPT_REDIR_PROTOCOLS_STR — allowed redirect protocols ("https") [>=7.85.0]
  CURLOPT_PROTOCOLS           — allowed protocols (CURLPROTO_HTTPS) [fallback]
  CURLOPT_REDIR_PROTOCOLS     — allowed redirect protocols (CURLPROTO_HTTPS) [fallback]

[files depende on this dependencie]
L0-core/src/DBResolver.cpp      — downloadDB() (curl_easy_* transfer API)
CoreRunner.cpp                   — main() bootstrap (curl_global_init/cleanup)

[code snippet]
bool DBResolver::downloadDB(const std::string& url, const std::string& destPath) {
    CURL* curl = curl_easy_init();
    if (!curl) return false;

    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER,  1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST,  2L);
    curl_easy_setopt(curl, CURLOPT_SSLVERSION,      CURL_SSLVERSION_TLSv1_2);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR,   "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,         30L);
    curl_easy_setopt(curl, CURLOPT_MAXFILESIZE,     20971520L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    // ... error handling ...
    return res == CURLE_OK;
}

[cause]
libcurl is the only networking dependency in the project. It provides:
  • DBResolver::downloadDB()  — downloads the official tguide.db database
    file from the project's HTTPS release URL over the internet
  • CoreRunner::main()        — curl_global_init() / curl_global_cleanup()
    bootstrap lifecycle

libcurl was chosen over raw sockets+OpenSSL because it provides a
battle-tested, cross-platform API with built-in TLS, redirect handling,
and timeout management — reducing the networking code to ~50 lines.

The dependency is declared via find_package(CURL REQUIRED) and linked as
PUBLIC on the L0_core static library so that any target consuming L0_core
(notably the tguide executable) transitively inherits curl's include
directories and link flags.
```

---

## 3. nlohmann/json (Bundled Header-Only Library)

```
[name]
nlohmann/json (JSON parsing, manipulation, and serialization)

[fonctions used]
json::object()                  — create an empty JSON object
json::array()                   — create an empty JSON array
json::dump(indent)              — serialize to indented string
json::parse(string)             — deserialize from string
json::contains(key)             — check if a key exists
json::at(key)                   — safe access to a key (throws on missing)
json::value(key, default)       — access with fallback default
json::is_null()                 — check if value is null
json::is_string()               — check if value is a string
json::is_object()               — check if value is an object
json::is_array()                — check if value is an array
json::items()                   — iterate key-value pairs
json::begin() / json::end()     — iterator access
json::erase(it)                 — remove element at iterator
json::push_back(value)          — append to array
json::back()                    — access the last element
operator[](key)                 — keyed or indexed access
get<Type>()                     — type-safe value extraction
operator<< / operator>>         — stream I/O for reading/writing files
nlohmann::json                  — underlying JSON value type
using json = nlohmann::json     — convenience typedef used project-wide

[files depende on this dependencie]
L0-core/include/config_manager.h
L0-core/include/db_cache_manager.h
L0-core/src/config_manager.cpp
L0-core/src/db_cache_manager.cpp
L0-core/src/UserDataManager.cpp

[code snippet]
void ConfigManager::merge(json& target, const json& defaults) {
    for (auto it = defaults.begin(); it != defaults.end(); ++it) {
        const auto& key = it.key();
        const auto& value = it.value();
        if (!target.contains(key)
            || target[key].is_null()
            || (target[key].is_string() && target[key].get<std::string>().empty())) {
            target[key] = value;
        } else if (value.is_object()) {
            merge(target[key], value);
        }
    }
}

bool ConfigManager::load() {
    std::ifstream in(path);
    if (in.is_open()) {
        try { in >> config; }
        catch (...) { config = json::object(); }
    }
    if (config.is_null()) config = json::object();
    return true;
}

[cause]
nlohmann/json provides the JSON serialization/deserialization backbone for
three distinct subsystems: (1) ConfigManager reads/writes config.json for
application settings persistence, (2) DBCacheManager reads/writes a JSON
cache file for database integrity hash tracking, and (3) UserDataManager
reads/writes saved_commands.json and saved_scripts.json for user-generated
content. The header-only design requires zero compilation or linking, and
the intuitive syntax (operator[], .contains(), .dump()) keeps the ~200
lines of JSON-handling code readable and maintainable.
```

---

## 4. POSIX System API (Linux / macOS / Termux)

```
[name]
POSIX System API (<unistd.h>, <cstdlib>)

[fonctions used]
geteuid()              — get effective user ID (used for root detection)
isatty()               — test if a file descriptor refers to a terminal
std::getenv()          — read environment variables
fileno()               — get file descriptor from FILE* stream

[files depende on this dependencie]
L0-core/include/path_resolver.h      (std::getenv)
L0-core/include/UI_colors.h          (geteuid, isatty, fileno)
L0-core/src/DBResolver.cpp           (std::getenv)
L2-Interface_Engine/src/UI_utils.cpp (std::getenv)

[code snippet]
// UI_colors.h — runtime color capability detection
static bool colorCheck() {
#ifdef _WIN32
    return false;
#else
    return isatty(fileno(stdout)) != 0;
#endif
}

// path_resolver.h — environment-based path resolution
static std::string getHome() {
    const char* home = std::getenv("HOME");
    return home ? home : "";
}

[cause]
POSIX APIs handle cross-platform concerns that the C++ standard library
does not cover: geteuid() for root-detection at startup, isatty() for
color-support detection on terminals, and getenv() for resolving XDG-
compliant data/config paths on Linux. These are thin wrappers (~1-2 lines
each) that provide essential OS-level information without adding a
framework dependency.
```

---

## 5. Windows System API (Windows Only)

```
[name]
Windows System API (<windows.h>, <io.h>)

[fonctions used]
system("cls")          — clear the terminal screen on Windows
_isatty()              — Windows equivalent of POSIX isatty()
_fileno()              — Windows equivalent of POSIX fileno()

[files depende on this dependencie]
L2-Interface_Engine/src/UI_utils.cpp   (system("cls"))
L2-Interface_Engine/includes/UI_colors.h  (_isatty, _fileno)

[code snippet]
// UI_utils.cpp — cross-platform screen clear
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// UI_colors.h — Windows terminal detection
#ifdef _WIN32
#  include <io.h>
   return _isatty(_fileno(stdout)) != 0;
#endif

[cause]
The Windows API shim is required for the two platform-specific operations
that the C++ standard library does not abstract: clearing the terminal
screen (system("cls")) and detecting terminal color capability. The
Windows code paths are isolated behind #ifdef _WIN32 guards and are
never compiled on non-Windows targets. No other Windows-specific APIs
are used.
```

---

## 6. C++ Standard Library (Compiler-Provided)

```
[name]
C++ Standard Library (C++17)

[fonctions used]
std::string                — dynamic string storage (ubiquitous)
std::vector<T>             — dynamic array storage (ubiquitous)
std::filesystem::path      — cross-platform path representation
std::filesystem::exists()  — file/directory existence check
std::filesystem::remove()  — delete file
std::filesystem::copy_file() — copy file with overwrite options
std::filesystem::rename()  — rename/move file
std::filesystem::create_directories() — mkdir -p equivalent
std::function<...>         — callable wrapper for callbacks
std::optional<T>           — optional value semantics
std::unordered_map<K,V>    — hash map
std::set<T>                — ordered set
std::ifstream / std::ofstream — file I/O streams
std::stringstream          — in-memory string formatting
std::find_if / std::find / std::remove_if / std::transform — algorithms
std::cout / std::cin / std::cerr — console I/O
std::getline()             — line-based input
std::error_code            — filesystem error handling
std::memcmp()              — memory comparison (SHA-256)
uint8_t / uint32_t / uint64_t — fixed-width integer types
from_chars()               — low-level numeric parsing

[files depende on this dependencie]
All 25 .cpp files and 26 .h files in the project.

[code snippet]
// Filesystem usage (ubiquitous pattern across 5 files):
std::filesystem::path configDir = std::filesystem::path(home) / ".config" / "tguide";
if (!std::filesystem::exists(configDir))
    std::filesystem::create_directories(configDir);

// Optional usage (db_cache_manager.h):
std::optional<DBRecord> findByHash(const std::string& hash) const;

// Function usage (ErrorHandler.h):
struct ErrorHandler {
    std::function<void(const std::string&)> fatal;
    std::function<void(const std::string&)> error;
    std::function<void(const std::string&)> attention;
};

[cause]
The C++17 standard library provides the foundational runtime: string
handling, I/O streams, filesystem operations, data structures (vector,
set, unordered_map), algorithms, and type support (optional, function,
error_code). Every file in the project depends on at least one standard
library header. The project requires C++17 (enforced via
CMAKE_CXX_STANDARD 17 + CMAKE_CXX_STANDARD_REQUIRED ON) primarily for
std::filesystem, which replaced the previous ad-hoc path manipulation,
and for std::optional, which provides cleaner optional-value semantics
than raw pointers or sentinel values.
```

---

## Dependency Inventory Summary

| # | Name | Type | How Linked | Version | Files Using It |
|---|------|------|-----------|---------|---------------|
| 1 | **SQLite3** | Bundled C amalgamation | OBJECT library `sqlite3_obj` | 3.50.4 | 3 files |
| 2 | **libcurl** | System shared library | `find_package(CURL REQUIRED)` | ≥ 7.0 (deb) | 1 file |
| 3 | **nlohmann/json** | Bundled header-only C++ | Include path `${LIBS_DIR}` | (single-header) | 5 files |
| 4 | **POSIX API** | System headers | Implicit on Linux/macOS | N/A | 4 files |
| 5 | **Windows API** | System headers | Implicit on Windows | N/A | 2 files |
| 6 | **C++17 Standard Library** | Compiler-provided | Implicit (C++17 std) | C++17 | All 51 files |

**Total external dependencies: 3** (SQLite3, libcurl, nlohmann/json) + 2 platform SDKs + C++17 stdlib.

**Key architectural notes:**
- SHA256 (L0-core/src/sha256.cpp) is NOT an external dependency — it is a custom FIPS 180-4 implementation from scratch with no OpenSSL or crypto library dependency.
- No threading libraries (pthreads, std::thread) are used anywhere.
- No Boost libraries are used anywhere.
- No XML libraries are used anywhere.
- No compression libraries (zlib, etc.) are used anywhere.
- No other networking libraries beyond libcurl.
