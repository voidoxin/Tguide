# ==============================================================
# Test target: tguide_tests
# ==============================================================
set(TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_sha256.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_config_manager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/test_user_data_manager.cpp
)

add_executable(tguide_tests ${TEST_SOURCES})

target_include_directories(tguide_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/libs                         # doctest.h, json.hpp
    ${CMAKE_SOURCE_DIR}/L0-core/include              # sha256.h, config_manager.h, etc.
)

target_link_libraries(tguide_tests
    PRIVATE L0_core
)

add_test(NAME tguide_tests COMMAND tguide_tests)
