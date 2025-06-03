cmake_minimum_required(VERSION 3.14)
project(ink
    LANGUAGES CXX
    VERSION 1.0.0
    DESCRIPTION "Ink Library"
)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(INK_BUILD_TESTS "Build tests" ON)
option(INK_AUTO_INSTALL "Auto install lib" ON)

# Define include directories
include_directories(include)

# Find dependencies
set(ALL_LIBRARY_PATH "$ENV{LIBRARY_PATH}/lib")

find_package(Threads REQUIRED)
find_library(TBB_STATIC_LIB NAMES libtbb.a tbb HINTS ${ALL_LIBRARY_PATH})

# Source files
file(GLOB_RECURSE SOURCES
    ${CMAKE_SOURCE_DIR}/src/*.cpp
)

# Header files
file(GLOB_RECURSE HEADERS
    ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.h
    ${CMAKE_SOURCE_DIR}/include/${PROJECT_NAME}/*.hpp
)

# Define the library
add_library(${PROJECT_NAME} STATIC
    ${SOURCES}
    ${HEADERS}
)

# Set include directories for the target
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link dependencies
target_link_libraries(${PROJECT_NAME} PUBLIC
    Threads::Threads
    ${TBB_STATIC_LIB}
)

# Testing
if(INK_BUILD_TESTS)
    enable_testing()

    # Find all test source files
    file(GLOB TEST_SOURCES
        ${CMAKE_SOURCE_DIR}/test/*.cpp
    )

    # Create test executable
    add_executable(${PROJECT_NAME}_tests ${TEST_SOURCES})

    # Link the library to the test executable
    target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME})

    # Register test
    add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
endif()

# Compiler flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE -g -O0)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -O3 -DNDEBUG)
endif()

# Installation
include(GNUInstallDirs)

# Install the library binaries
install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install header files
install(
    DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Check if running with admin/root permissions
if(WIN32)
    # Windows: Check for admin rights (not perfect but a reasonable check)
    execute_process(
        COMMAND cmd /c "net session >nul 2>&1"
        RESULT_VARIABLE admin_result
        OUTPUT_QUIET
        ERROR_QUIET
    )
    set(IS_ADMIN ${admin_result} EQUAL 0)
else()
    # Unix-like: Check if effective user ID is 0 (root)
    execute_process(
        COMMAND bash -c "[ $EUID -eq 0 ]"
        RESULT_VARIABLE not_root_result
        OUTPUT_QUIET
        ERROR_QUIET
    )
    set(IS_ADMIN ${not_root_result} EQUAL 0)
endif()

# If we have admin rights, install as post-build step
if(INK_AUTO_INSTALL)
    message(STATUS "Auto-install enabled: Library will be installed after each build")
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -P cmake_install.cmake
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Auto-installing ${PROJECT_NAME} library..."
    )
endif()

# Print summary information
message(STATUS "=== ${PROJECT_NAME} Configuration Summary ===")
message(STATUS "Version:         ${PROJECT_VERSION}")
message(STATUS "Build type:      ${CMAKE_BUILD_TYPE}")
message(STATUS "Build tests:     ${INK_BUILD_TESTS}")
message(STATUS "Auto-install:    ${INK_AUTO_INSTALL}")
message(STATUS "Install prefix:  ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=========================================")
