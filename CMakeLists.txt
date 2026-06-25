cmake_minimum_required(VERSION 3.16)
project(tguide VERSION 1.0.1 LANGUAGES C CXX)

# ==============================================================
# Language Standards
# C++17 required for std::filesystem, std::optional
# C11  required for sqlite3.c (pure C library)
# ==============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
                                                  # ==============================================================                                    # Build Type
# Default to Release if not specified
# ==============================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# ==============================================================
# Dev Mode — enable dev-only features (BackupManager)
# remove before release build
# ==============================================================
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(TGUIDE_DEV_MODE_DEFAULT OFF)
else()
    set(TGUIDE_DEV_MODE_DEFAULT ON)
endif()
option(TGUIDE_DEV_MODE "Enable dev-only features" ${TGUIDE_DEV_MODE_DEFAULT})
if(TGUIDE_DEV_MODE)
    add_compile_definitions(TGUIDE_DEV_MODE)
    message(STATUS "Dev mode: ON — BackupManager active")
else()
    message(STATUS "Dev mode: OFF — release build")
endif()

# ==============================================================
# Compiler Flags
# ==============================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    message(STATUS "Compiler: GCC ${CMAKE_CXX_COMPILER_VERSION}")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O2)
        add_compile_definitions(NDEBUG)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|IntelLLVM")
    message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-O2)
        add_compile_definitions(NDEBUG)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    message(STATUS "Compiler: MSVC ${CMAKE_CXX_COMPILER_VERSION}")
    add_compile_options(/W4 /utf-8)
else()
    message(WARNING "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

# ==============================================================
# MSVC-specific definitions — prevent Windows header conflicts
# NOMINMAX avoids min/max macro collision with std::min/std::max
# _CRT_SECURE_NO_WARNINGS suppresses deprecation warnings for
# standard C functions (used by sqlite3.c)
# ==============================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" OR CMAKE_CXX_SIMULATE_ID MATCHES "MSVC")
    add_compile_definitions(NOMINMAX _CRT_SECURE_NO_WARNINGS)
endif()

# ==============================================================
# Platform / Environment Detection
# Termux sets $PREFIX containing "com.termux" — detected at
# cmake time via environment variable
# ==============================================================
if(DEFINED ENV{PREFIX} AND "$ENV{PREFIX}" MATCHES "com.termux")
    set(TGUIDE_PLATFORM "termux")
    set(IS_TERMUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(TGUIDE_PLATFORM "linux")
    set(IS_LINUX TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(TGUIDE_PLATFORM "windows")
    set(IS_WINDOWS TRUE)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(TGUIDE_PLATFORM "macos")
    set(IS_MACOS TRUE)
else()
    set(TGUIDE_PLATFORM "unknown")
endif()

message(STATUS "Platform: ${TGUIDE_PLATFORM}")
add_compile_definitions(TGUIDE_PLATFORM="${TGUIDE_PLATFORM}")

# Program version — used by cli_parser.cpp for --version output
add_compile_definitions(TGUIDE_VERSION="${PROJECT_VERSION}")

# ==============================================================
# Install Paths — system-wide deployment locations
# Runtime paths use user-space (~/.config, ~/.local/share) via PathResolver
#
#   Linux   : binary → /usr/local/bin/
#             data   → /usr/local/share/tguide/    (package-manager installs only)
#             config → /usr/local/etc/tguide/      (package-manager installs only)
#
#   Termux  : binary → $PREFIX/bin/
#             data   → $PREFIX/share/tguide/
#             config → $PREFIX/etc/tguide/
#
#   Windows : binary → %PROGRAMDATA%\tguide\bin\
#             data   → %PROGRAMDATA%\tguide\    (matches PathResolver::installDbFile())
#             config → %PROGRAMDATA%\tguide\
# ==============================================================
if(IS_TERMUX)
    set(TGUIDE_INSTALL_BIN    "$ENV{PREFIX}/bin")
    set(TGUIDE_INSTALL_DATA   "$ENV{PREFIX}/share/tguide")
    set(TGUIDE_INSTALL_CONFIG "$ENV{PREFIX}/etc/tguide")
elseif(IS_LINUX)
    set(TGUIDE_INSTALL_BIN    "/usr/local/bin")
    set(TGUIDE_INSTALL_DATA   "/usr/local/share/tguide")
    set(TGUIDE_INSTALL_CONFIG "/usr/local/etc/tguide")
elseif(IS_WINDOWS)
    # Windows: system-wide install under %PROGRAMDATA% matches
    # PathResolver::installDbFile() which returns %PROGRAMDATA%/tguide/tguide.db
    # Fallback to C:\ProgramData if env var is unset (rare but safe)
    if(DEFINED ENV{PROGRAMDATA})
        set(_PGDATA "$ENV{PROGRAMDATA}")
    else()
        message(WARNING "PROGRAMDATA not set — falling back to C:\\ProgramData")
        set(_PGDATA "C:\\ProgramData")
    endif()
    set(TGUIDE_INSTALL_BIN    "${_PGDATA}/tguide/bin")
    set(TGUIDE_INSTALL_DATA   "${_PGDATA}/tguide")
    set(TGUIDE_INSTALL_CONFIG "${_PGDATA}/tguide")
elseif(IS_MACOS)
    set(TGUIDE_INSTALL_BIN    "/usr/local/bin")
    set(TGUIDE_INSTALL_DATA   "/Library/Application Support/tguide")
    set(TGUIDE_INSTALL_CONFIG "/Library/Application Support/tguide")
endif()

# ==============================================================
# Install RPATH — ensures the installed binary can find linked
# shared libraries (e.g. libcurl) at runtime
# ==============================================================
if(CMAKE_INSTALL_RPATH)
    # Respect any RPATH already set by the build environment
else()
    set(CMAKE_INSTALL_RPATH "${TGUIDE_INSTALL_BIN}/../lib")
endif()
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ==============================================================
# Source Paths
# ==============================================================
set(LIBS_DIR   ${CMAKE_SOURCE_DIR}/libs)
set(L0_INC     ${CMAKE_SOURCE_DIR}/L0-core/include)
set(L1_INC     ${CMAKE_SOURCE_DIR}/L1-services/includes)
set(L2_INC     ${CMAKE_SOURCE_DIR}/L2-services/includes)
set(L3_INC     ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/includes)
set(OBJECT_DIR ${CMAKE_SOURCE_DIR}/object)

# ==============================================================
# sqlite3 — pure C, compiled separately
# Always recompile from source for reliability
# ==============================================================
set(SQLITE3_SRC ${LIBS_DIR}/sqlite3.c)

add_library(sqlite3_obj OBJECT ${SQLITE3_SRC})
set_target_properties(sqlite3_obj PROPERTIES
    LINKER_LANGUAGE C
    C_STANDARD 11
)
target_include_directories(sqlite3_obj PRIVATE ${LIBS_DIR})
target_compile_options(sqlite3_obj PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang,AppleClang>:-O0>
    $<$<C_COMPILER_ID:MSVC>:/Od>
)

# ==============================================================
# json.hpp — header-only (nlohmann/json)
# ==============================================================
if(NOT EXISTS ${LIBS_DIR}/json.hpp)
    message(FATAL_ERROR
        "json.hpp not found in ${LIBS_DIR}.\n"
        "Download from: https://github.com/nlohmann/json/releases"
    )
else()
    message(STATUS "json.hpp found")
endif()

# ==============================================================
# libcurl — required for DB download fallback
#   Debian/Kali : apt install libcurl4-openssl-dev
#   Termux      : pkg install libcurl
#   Arch        : pacman -S curl
#   Windows     : vcpkg install curl
# ==============================================================
find_package(CURL REQUIRED)
message(STATUS "libcurl: ${CURL_VERSION_STRING}")

# ==============================================================
# L0-core — foundation layer
# database, config, cache, sha256
# ==============================================================
set(L0_SOURCES
    ${CMAKE_SOURCE_DIR}/L0-core/src/cli_parser.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/session_flags.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/DatabaseManager.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/DBResolver.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/ErrorHandler.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/config_manager.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/db_cache_manager.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/sha256.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/string_utils.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/UserDataManager.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/logger.cpp
    ${CMAKE_SOURCE_DIR}/L0-core/src/paginator.cpp
)

add_library(L0_core STATIC ${L0_SOURCES})

target_include_directories(L0_core PUBLIC
    ${L0_INC}
    ${LIBS_DIR}
)

target_link_libraries(L0_core
    PRIVATE sqlite3_obj
    PUBLIC  CURL::libcurl
)

# ==============================================================
# L1-services — business logic layer
# ==============================================================
set(L1_SOURCES
    ${CMAKE_SOURCE_DIR}/L1-services/src/svc_tools.cpp
    ${CMAKE_SOURCE_DIR}/L1-services/src/svc_generator.cpp
    ${CMAKE_SOURCE_DIR}/L1-services/src/svc_savedCommands.cpp
    ${CMAKE_SOURCE_DIR}/L1-services/src/svc_savedTemplates.cpp
    ${CMAKE_SOURCE_DIR}/L1-services/src/svc_savedScripts.cpp
    ${CMAKE_SOURCE_DIR}/L1-services/src/svc_settings.cpp
)

add_library(L1_services STATIC ${L1_SOURCES})
target_include_directories(L1_services PUBLIC ${L1_INC})
target_include_directories(L1_services PRIVATE ${L0_INC} ${LIBS_DIR})
target_link_libraries(L1_services PRIVATE L0_core)

# ==============================================================
# L2-services — service / business logic layer (empty for now)
# bridges L2-Interface_Engine screens to L0-core queries
# ==============================================================
# set(L2_SOURCES
#     ${CMAKE_SOURCE_DIR}/L2-services/src/svc_tools.cpp
#     ${CMAKE_SOURCE_DIR}/L2-services/src/svc_generator.cpp
#     ${CMAKE_SOURCE_DIR}/L2-services/src/svc_savedCommands.cpp
#     ${CMAKE_SOURCE_DIR}/L2-services/src/svc_savedScripts.cpp
#     ${CMAKE_SOURCE_DIR}/L2-services/src/svc_settings.cpp
# )
# add_library(L2_services STATIC ${L2_SOURCES})
# target_include_directories(L2_services PUBLIC ${L2_INC})
# target_link_libraries(L2_services PRIVATE L0_core)

# ==============================================================
# L2-Interface_Engine — UI and error handling
# Implements UI_attention(), UI_errors(), UI_fatal()
# ==============================================================
set(L3_SOURCES
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_Engine.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_errorHandling.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_utils.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_input.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_paginator.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_tools.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_generator.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_savedCommands.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_savedScripts.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_settings.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_disclaimer.cpp
)

add_library(L3_interface STATIC ${L3_SOURCES})

target_include_directories(L3_interface PUBLIC
    ${L3_INC}
    ${L1_INC}
)
target_include_directories(L3_interface PRIVATE
    ${L0_INC}
    ${LIBS_DIR}
)

target_link_libraries(L3_interface PRIVATE L1_services)

# ==============================================================
# ==============================================================
# tguide — main binary
# ==============================================================
add_executable(tguide
    ${CMAKE_SOURCE_DIR}/CoreRunner.cpp
    ${CMAKE_SOURCE_DIR}/cli_display.cpp
    ${CMAKE_SOURCE_DIR}/cli_export.cpp
)

set_target_properties(tguide PROPERTIES OUTPUT_NAME "tguide")

target_include_directories(tguide PRIVATE
    ${L0_INC}
    ${L1_INC}
    ${L3_INC}
    ${LIBS_DIR}
)

target_link_libraries(tguide
    PRIVATE L0_core
    PRIVATE L1_services
    # PRIVATE L2_services
    PRIVATE L3_interface
)

# ==============================================================
# Install Targets
# run: sudo cmake --install . (Linux)
#           cmake --install . (Termux / Windows / macOS)
# ==============================================================
if(IS_LINUX OR IS_TERMUX OR IS_MACOS OR IS_WINDOWS)

    # binary — install on all platforms
    install(TARGETS tguide
        RUNTIME DESTINATION ${TGUIDE_INSTALL_BIN}
    )

    # database & config — all Unix platforms
    # Data files are committed alongside the source code, not generated at build time.
    # The seed DB is a pre-built SQLite file (tguide.db) committed to the repo.
    if(EXISTS ${CMAKE_SOURCE_DIR}/data/database/tguide.db)
        install(FILES ${CMAKE_SOURCE_DIR}/data/database/tguide.db
            DESTINATION ${TGUIDE_INSTALL_DATA}
        )
    else()
        message(WARNING
            "tguide.db not found in data/database/ — "
            "it will not be included in the install. "
            "The app will attempt to download it on first run."
        )
    endif()

    if(EXISTS ${CMAKE_SOURCE_DIR}/config/config.json)
        install(FILES ${CMAKE_SOURCE_DIR}/config/config.json
            DESTINATION ${TGUIDE_INSTALL_CONFIG}
        )
    endif()

endif()

# ==============================================================
# Testing — CTest integration
# ==============================================================
if(NOT TGUIDE_SKIP_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ==============================================================
# CPack — .deb package generation (Linux only)
# usage: cpack -G DEB
# ==============================================================
if(IS_LINUX)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_PACKAGE_NAME "tguide")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_CONTACT "voidoxin")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "tguide — security tools reference for penetration testers"
    )
    set(CPACK_DEBIAN_PACKAGE_DEPENDS
        "libcurl4 (>= 7.0)"
    )
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
    set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    include(CPack)
endif()

# ==============================================================
# CPack — macOS Bundle generation
# usage: cpack -G Bundle
# ==============================================================
if(IS_MACOS)
    set(CPACK_GENERATOR "Bundle")
    set(CPACK_PACKAGE_NAME "tguide")
    set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
    set(CPACK_PACKAGE_CONTACT "voidoxin")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY
        "tguide — security tools reference for penetration testers"
    )
    set(CPACK_BUNDLE_NAME "tguide")
    include(CPack)
endif()

# ==============================================================
# Directory stubs — preserve empty folders in version control
# ==============================================================
foreach(EMPTY_DIR
    ${CMAKE_SOURCE_DIR}/L1-services/includes
    ${CMAKE_SOURCE_DIR}/L1-services/src
    ${CMAKE_SOURCE_DIR}/L2-services/includes
    ${CMAKE_SOURCE_DIR}/L2-services/src
    ${CMAKE_SOURCE_DIR}/data/backup
    ${CMAKE_SOURCE_DIR}/scripts
    ${OBJECT_DIR}
)
    if(NOT EXISTS ${EMPTY_DIR})
        file(MAKE_DIRECTORY ${EMPTY_DIR})
        message(STATUS "Created directory: ${EMPTY_DIR}")
    endif()
endforeach()

# ==============================================================
# Build Summary
# ==============================================================
message(STATUS "")
message(STATUS "============= tguide Build Configuration =============")
message(STATUS "  Version    : ${PROJECT_VERSION}")
message(STATUS "  Platform   : ${TGUIDE_PLATFORM}")
message(STATUS "  Compiler   : ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Build type : ${CMAKE_BUILD_TYPE}")
message(STATUS "  libcurl    : ${CURL_VERSION_STRING}")
message(STATUS "  Install    : ${TGUIDE_INSTALL_BIN}")
message(STATUS "  Data       : ${TGUIDE_INSTALL_DATA}")
message(STATUS "  Config     : ${TGUIDE_INSTALL_CONFIG}")
message(STATUS "======================================================")
message(STATUS "")