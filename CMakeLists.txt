cmake_minimum_required(VERSION 3.14)

# Add project_options v0.20.0
# https://github.com/cpp-best-practices/project_options
include(FetchContent)
FetchContent_Declare(
    _project_options 
    URL https://github.com/aminya/project_options/archive/refs/tags/v0.20.0.zip
)
FetchContent_MakeAvailable(_project_options)

FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG ad0e89cbfb4d0c1ce4d097e134eb7be67baebb36 # 1.11.0
)

FetchContent_MakeAvailable(spdlog)

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

add_library(rateless_codes
    include/rlf.h
    src/rlf.cpp
)
target_include_directories(rateless_codes PUBLIC include)
target_link_libraries(rateless_codes PRIVATE spdlog::spdlog)
# I wish it was simpler one day:
# https://gitlab.kitware.com/cmake/cmake/-/issues/23246
# https://discourse.cmake.org/t/mt-staticrelease-doesnt-match-value-md-dynamicrelease/5428/3
if(MSVC)
    set_property(TARGET rateless_codes PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

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
    FetchContent_Declare(
	googletest
	GIT_REPOSITORY https://github.com/google/googletest.git
	GIT_TAG        58d77fa8070e8cec2dc1ed015d66b454c8d78850 # release-1.12.1
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_executable(main 
	rlf.cc
    )

    target_link_libraries(main 
	PRIVATE 
	    rateless_codes 
	    spdlog::spdlog
	    gtest_main 
	    project_options 
	    project_warnings
    )
    gtest_discover_tests(main)

    if(RATELESS_CODES_ENABLE_FUZZING)
#	message(STATUS "Fuzzing enabled")
#	add_subdirectory(fuzzer)
    endif()
endif()
