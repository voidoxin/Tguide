cmake_minimum_required(VERSION 3.16)

project(tguide VERSION 1.0 LANGUAGES CXX C)

# ===== Settings =====
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ===== Global Include =====
include_directories(${PROJECT_SOURCE_DIR}/libs)

# =========================================================
# ===================== L0 CORE ============================
# =========================================================

add_library(L0_core
    L0-core/src/DatabaseManager.cpp
    L0-core/src/config_manager.cpp
    libs/sqlite3.c
)

target_include_directories(L0_core PUBLIC
    ${PROJECT_SOURCE_DIR}/L0-core/include
    ${PROJECT_SOURCE_DIR}/libs
)

# =========================================================
# ===================== L1 SERVICES ========================
# =========================================================

file(GLOB L1_SOURCES
    L1-services/src/*.cpp
)

add_library(L1_services ${L1_SOURCES})

target_include_directories(L1_services PUBLIC
    ${PROJECT_SOURCE_DIR}/L1-services/includes
)

target_link_libraries(L1_services PUBLIC L0_core)

# =========================================================
# ===================== L3 INTERFACE =======================
# =========================================================

file(GLOB L3_SOURCES
    L3-Interface_Engine/src/*.cpp
)

add_library(L3_interface ${L3_SOURCES})

target_include_directories(L3_interface PUBLIC
    ${PROJECT_SOURCE_DIR}/L3-Interface_Engine/includes
)

target_link_libraries(L3_interface PUBLIC L1_services)

# =========================================================
# ===================== EXECUTABLE =========================
# =========================================================

add_executable(CoreRunner CoreRunner.cpp)

target_link_libraries(CoreRunner
    L3_interface
    L1_services
    L0_core
)

# =========================================================
# ===================== PLATFORM ===========================
# =========================================================

if(UNIX)
    target_link_libraries(CoreRunner dl pthread)
endif()

# =========================================================
# ===================== WARNINGS ===========================
# =========================================================

if(MSVC)
    target_compile_options(CoreRunner PRIVATE /W4)
else()
    target_compile_options(CoreRunner PRIVATE -Wall -Wextra -Wpedantic)
endif()
