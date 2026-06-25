# ==============================================================
# Test target: tguide_tests
# ==============================================================
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sha256.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_strings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_db_recovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_path_resolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_search.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user_data_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_saved_commands.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_saved_scripts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_cli_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_cli_display.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_paginator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ui_input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_ui_quit.cpp
    ${CMAKE_SOURCE_DIR}/cli_display.cpp
    ${CMAKE_SOURCE_DIR}/cli_export.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_input.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_utils.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_disclaimer.cpp
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/src/UI_errorHandling.cpp
)

add_executable(tguide_tests ${TEST_SOURCES})

target_include_directories(tguide_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/libs                         # doctest.h, json.hpp
    ${CMAKE_SOURCE_DIR}/L0-core/include              # sha256.h, config_manager.h, etc.
    ${CMAKE_SOURCE_DIR}/L1-services/includes         # svc_savedCommands.h, svc_dto.h
    ${CMAKE_SOURCE_DIR}/L2-Interface_Engine/includes # UI_input.h
    ${CMAKE_SOURCE_DIR}                              # cli_display.h
)

# Expose source tree root so tests can load the committed seed DB
target_compile_definitions(tguide_tests PRIVATE
    TGUIDE_SOURCE_DIR="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(tguide_tests
    PRIVATE L0_core
    PRIVATE L1_services
)

add_test(NAME tguide_tests COMMAND tguide_tests)
