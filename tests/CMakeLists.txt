cmake_minimum_required(VERSION 3.10)
project(cfs_test LANGUAGES C)

enable_testing()

option(PRINT_ENV  "Print the test environment at the start" ON)
option(USE_COLORS "Use colors in tests" OFF)

function(add_cfs_test target source)
    add_executable(${target} "${source}")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../include")

    set_target_properties(${target} PROPERTIES
            C_STANDARD 11
            C_STANDARD_REQUIRED ON)

    if (NOT WIN32)
        target_compile_definitions(${target} PRIVATE _GNU_SOURCE)
    endif ()

    if (MSVC)
        target_compile_definitions(${target} PRIVATE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(${target} PRIVATE
                "/wd4668" # Disabled due to WinAPI
                "/wd4996" # Deprecation warning
                "/wd4820" # Implicit padding
                "/wd5045" # Spectre mitigation
        )
    endif ()

    if (USE_COLORS)
        target_compile_definitions(${target} PRIVATE FS_TESTS_USE_COLORS)
    endif ()

    if (PRINT_ENV)
        target_compile_definitions(${target} PRIVATE FS_TEST_PRINT_ENV)
    endif ()
endfunction ()

add_cfs_test(cfs_test "main.c")
target_compile_definitions(cfs_test PRIVATE _TEST_ROOT="${CMAKE_CURRENT_LIST_DIR}/.TestRoot")

add_test(NAME cfs_test COMMAND cfs_test)
set_tests_properties(cfs_test PROPERTIES WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}")

add_cfs_test(boost_operations_smoke_test "boost_operations_smoke_test.c")
add_test(NAME boost.operations_smoke COMMAND boost_operations_smoke_test)
set_tests_properties(boost.operations_smoke PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LABELS "boost;operations;smoke")

add_cfs_test(boost_path_lexical_test "boost_path_lexical_test.c")
add_test(NAME boost.path_lexical COMMAND boost_path_lexical_test)
set_tests_properties(boost.path_lexical PROPERTIES
        LABELS "boost;path;lexical")

add_cfs_test(boost_path_modifiers_test "boost_path_modifiers_test.c")
add_test(NAME boost.path_modifiers COMMAND boost_path_modifiers_test)
set_tests_properties(boost.path_modifiers PROPERTIES
        LABELS "boost;path;modifiers")

add_cfs_test(boost_copy_test "boost_copy_test.c")
add_test(NAME boost.copy COMMAND boost_copy_test)
set_tests_properties(boost.copy PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LABELS "boost;copy;operations")

add_cfs_test(boost_operations_test "boost_operations_test.c")
add_test(NAME boost.operations COMMAND boost_operations_test)
set_tests_properties(boost.operations PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LABELS "boost;operations")

add_cfs_test(boost_long_path_test "boost_long_path_test.c")
add_test(NAME boost.long_path COMMAND boost_long_path_test)
set_tests_properties(boost.long_path PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        LABELS "boost;path;long")
