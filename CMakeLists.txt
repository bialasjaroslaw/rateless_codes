cmake_minimum_required(VERSION 3.15)

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

# Add project_options v0.20.0
# https://github.com/cpp-best-practices/project_options
include(FetchContent)
FetchContent_Declare(
    _project_options 
    URL https://github.com/aminya/project_options/archive/refs/tags/v0.20.0.zip
)
FetchContent_MakeAvailable(_project_options)

include(${_project_options_SOURCE_DIR}/Index.cmake)

project(rateless_codes VERSION 0.2 LANGUAGES CXX)
option(ENABLE_DEVELOPER_MODE "Enable developer mode" OFF)
option(RATELESS_CODES_ENABLE_TESTS "Enable test build" ON)

set(ENABLE_SANITIZER_ADDRESS "ENABLE_SANITIZER_ADDRESS")
if(MSVC)
    string(FIND "$ENV{PATH}" "$ENV{VSINSTALLDIR}" index_of_vs_install_dir)
    if(NOT "${index_of_vs_install_dir}" STREQUAL "-1")
        set(ENABLE_SANITIZER_ADDRESS "")
    endif()
endif()

include(${_project_options_SOURCE_DIR}/src/DynamicProjectOptions.cmake)
dynamic_project_options(
    ENABLE_CACHE
    ENABLE_CPPCHECK
    ENABLE_CLANG_TIDY
    ${ENABLE_SANITIZER_ADDRESS}
    ENABLE_SANITIZER_LEAK
    ENABLE_SANITIZER_UNDEFINED_BEHAVIOR
    ENABLE_COVERAGE
)

find_package(spdlog CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)

set(SOURCES
    src/rlf.cpp
    src/lt.cpp
    src/node.cpp
    src/degree_distribution.cpp
    src/ideal_soliton_distribution.cpp
    src/robust_soliton_distribution.cpp
)

set(HEADERS
    include/rlf.h
    include/lt.h
    include/node.h
    include/degree_distribution.h
    include/ideal_soliton_distribution.h
    include/robust_soliton_distribution.h
)

add_library(rateless_codes
    ${SOURCES} ${HEADERS}
)
target_include_directories(rateless_codes PUBLIC include)
target_link_libraries(rateless_codes PRIVATE spdlog::spdlog)

if (RATELESS_CODES_ENABLE_TESTS AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(STATUS "Compiler id is ${CMAKE_CXX_COMPILER_ID}")
    option(RATELESS_CODES_ENABLE_FUZZING "Build fuzzing tests" ON)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

if(RATELESS_CODES_ENABLE_TESTS)
    include(FetchContent)
    include(CTest)
    include(GoogleTest)

    enable_testing()

    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

    add_executable(main 
        rlf.cc
        lt.cc
    )

    target_link_libraries(main 
    PRIVATE
        rateless_codes
        spdlog::spdlog
        GTest::gtest_main
        project_options
        project_warnings
    )
    gtest_discover_tests(main)

    if(RATELESS_CODES_ENABLE_FUZZING)
#	message(STATUS "Fuzzing enabled")
#	add_subdirectory(fuzzer)
    endif()
endif()
