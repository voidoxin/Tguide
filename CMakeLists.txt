cmake_minimum_required(VERSION 3.15)
project(tguide VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include paths
include_directories(
    L0-core/include
    libs
)

# Source files
set(SRC_FILES
    CoreRunner.cpp
    L0-core/src/DatabaseManager.cpp
    L0-core/src/config_manager.cpp
)

# Build executable
add_executable(CoreRunner ${SRC_FILES})

# Link libraries
target_link_libraries(CoreRunner
    pthread
    dl
)
