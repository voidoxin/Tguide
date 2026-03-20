cmake_minimum_required(VERSION 3.16)
project(ToolsGuide VERSION 1.0.1 LANGUAGES C CXX)

# ==============================================================
# Language Standards
# C++17 required for std::filesystem, std::optional
# C11 required for sqlite3.c (pure C library)
# ==============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# ==============================================================
# Compiler Detection
# Supports: GCC, Clang, MSVC
# Flags are set per-compiler to ensure broad compatibility
# ==============================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(STATUS "Compiler: Clang ${CMAKE_CXX_COMPILER_VERSION}")
    add_compile_options(-Wall -Wextra -Wpedantic)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compiler: MSVC ${CMAKE_CXX_COMPILER_VERSION}")
    add_compile_options(/W4)
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID} — proceeding without extra flags")
endif()

# ==============================================================
# Platform / Environment Detection
# Stored in a variable and passed as a compile definition
# so the application can read it at runtime via DB config
# ==============================================================
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TGUIDE_OS "linux")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Android")
    set(TGUIDE_OS "android")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TGUIDE_OS "windows")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TGUIDE_OS "macos")
else()
    set(TGUIDE_OS "unknown")
endif()
message(STATUS "Target platform: ${TGUIDE_OS}")
add_compile_definitions(TGUIDE_OS="${TGUIDE_OS}")

# ==============================================================
# Paths
# ==============================================================
set(LIBS_DIR     ${CMAKE_SOURCE_DIR}/libs)
set(L0_INC       ${CMAKE_SOURCE_DIR}/L0-core/include)
set(L1_INC       ${CMAKE_SOURCE_DIR}/L1-services/includes)
set(L3_INC       ${CMAKE_SOURCE_DIR}/L3-Interface_Engine/includes)
set(OBJECT_DIR   ${CMAKE_SOURCE_DIR}/object)

# ==============================================================
# sqlite3 — pure C library, must be compiled with a C compiler
# Compiled separately into an object file to avoid mixing
# C and C++ compilation flags on the same translation unit
# Check first: only compile if not already built
# ==============================================================
set(SQLITE3_SRC  ${LIBS_DIR}/sqlite3.c)
set(SQLITE3_OBJ  ${OBJECT_DIR}/sqlite3.o)

if(NOT EXISTS ${SQLITE3_OBJ})
    message(STATUS "sqlite3.o not found — will compile sqlite3.c as C object")
    add_library(sqlite3_obj OBJECT ${SQLITE3_SRC})
    set_target_properties(sqlite3_obj PROPERTIES
        LINKER_LANGUAGE C
        C_STANDARD 11
    )
    target_include_directories(sqlite3_obj PRIVATE ${LIBS_DIR})
else()
    message(STATUS "sqlite3.o already exists — skipping recompilation")
    add_library(sqlite3_obj IMPORTED STATIC GLOBAL)
    set_target_properties(sqlite3_obj PROPERTIES
        IMPORTED_LOCATION ${SQLITE3_OBJ}
    )
endif()

# ==============================================================
# json.hpp — header-only library (nlohmann/json)
# No compilation needed — just needs to be in include path
# Check that it exists before proceeding
# ==============================================================
if(NOT EXISTS ${LIBS_DIR}/json.hpp)
    message(FATAL_ERROR
        "json.hpp not found in ${LIBS_DIR}. "
        "Download it from: https://github.com/nlohmann/json/releases "
        "and place it in the libs/ directory."
    )
else()
    message(STATUS "json.hpp found in ${LIBS_DIR}")
endif()

# ==============================================================
# libcurl — required for database download fallback (Case 3b)
# Must be installed on the system:
#   Debian/Ubuntu/Kali : apt install libcurl4-openssl-dev
#   Termux             : pkg install libcurl
#   Arch               : pacman -S curl
#   Windows (vcpkg)    : vcpkg install curl
# ==============================================================
find_package(CURL REQUIRED)
if(CURL_FOUND)
    message(STATUS "libcurl found: ${CURL_VERSION_STRING}")
else()
    message(FATAL_ERROR
        "libcurl not found. Install it before building:\n"
        "  Debian/Kali : apt install libcurl4-openssl-dev\n"
        "  Termux      : pkg install libcurl\n"
        "  Arch        : pacman -S curl"
    )
endif()

# ==============================================================
# L0-core — database, config, cache, sha256
# This is the foundation layer — no UI or service dependencies
# ==============================================================
set(L0_SOURCES
    ${CMAKE_SOURCE_DIR}/L0-core/src/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/db_cache_manager.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/sha256.cpp
)

add_library(L0_core STATIC ${L0_SOURCES})

target_include_directories(L0_core PUBLIC
    ${L0_INC}
    ${LIBS_DIR}
)

target_link_libraries(L0_core
    PRIVATE sqlite3_obj
    PRIVATE CURL::libcurl
)

# ==============================================================
# L1-services — business logic layer
# Currently empty — add sources here when the layer is populated
# Depends on L0-core
# ==============================================================
# Placeholder — uncomment and populate when L1 sources are added:
#
# set(L1_SOURCES
#     ${CMAKE_SOURCE_DIR}/L1-services/src/example.cpp
# )
# add_library(L1_services STATIC ${L1_SOURCES})
# target_include_directories(L1_services PUBLIC ${L1_INC})
# target_link_libraries(L1_services PRIVATE L0_core)

# ==============================================================
# L3-Interface_Engine — UI and error handling layer
# Implements UI_attention(), UI_errors(), UI_fatal()
# which are declared extern in DatabaseManager.cpp
# Depends on L0-core (and L1 when populated)
# ==============================================================
set(L3_SOURCES
    ${CMAKE_SOURCE_DIR}/L3-Interface_Engine/src/UI_Engine.cpp
    ${CMAKE_SOURCE_DIR}/L3-Interface_Engine/src/UI_errorHandling.cpp
)

add_library(L3_interface STATIC ${L3_SOURCES})

target_include_directories(L3_interface PUBLIC
    ${L3_INC}
    ${L0_INC}
    ${LIBS_DIR}
)

target_link_libraries(L3_interface PRIVATE L0_core)

# ==============================================================
# data_adder — standalone utility binary
# Used to populate the database during development
# Not part of the main application
# ==============================================================
add_executable(data_adder ${CMAKE_SOURCE_DIR}/data_adder.cpp)

target_include_directories(data_adder PRIVATE
    ${L0_INC}
    ${L3_INC}
    ${LIBS_DIR}
)

target_link_libraries(data_adder
    PRIVATE L0_core
    PRIVATE L3_interface
)

# ==============================================================
# CoreRunner — main application entry point
# Links all layers together
# ==============================================================
add_executable(CoreRunner ${CMAKE_SOURCE_DIR}/CoreRunner.cpp)

target_include_directories(CoreRunner PRIVATE
    ${L0_INC}
    ${L1_INC}
    ${L3_INC}
    ${LIBS_DIR}
)

target_link_libraries(CoreRunner
    PRIVATE L0_core
    # PRIVATE L1_services   # uncomment when L1 is populated
    PRIVATE L3_interface
)

# ==============================================================
# Empty directory stubs
# CMake ignores empty directories — these .gitkeep targets
# ensure the folder structure is preserved in version control
# without affecting the build
# ==============================================================
foreach(EMPTY_DIR
    ${CMAKE_SOURCE_DIR}/L1-services/includes
    ${CMAKE_SOURCE_DIR}/L1-services/src
    ${CMAKE_SOURCE_DIR}/data/backup
    ${CMAKE_SOURCE_DIR}/scripts
    ${OBJECT_DIR}
)
    if(NOT EXISTS ${EMPTY_DIR})
        file(MAKE_DIRECTORY ${EMPTY_DIR})
        message(STATUS "Created empty directory: ${EMPTY_DIR}")
    endif()
endforeach()

# ==============================================================
# Build summary
# ==============================================================
message(STATUS "")
message(STATUS "======= ToolsGuide Build Configuration =======")
message(STATUS "  Version    : ${PROJECT_VERSION}")
message(STATUS "  Platform   : ${TGUIDE_OS}")
message(STATUS "  Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  C++ Std    : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  libcurl    : ${CURL_VERSION_STRING}")
message(STATUS "===============================================")
message(STATUS "")