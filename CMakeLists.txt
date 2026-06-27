cmake_minimum_required(VERSION 4.3.3)
project(ink
    VERSION 1.1.0
    DESCRIPTION "Ink Core Subsystem"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

# Core Build Options
option(INK_BUILD_TESTS "Build automated test suite" ON)
option(INK_ENABLE_LTO "Enable Interprocedural Optimization / LTO" ON)
option(INK_NATIVE_OPTIMIZE "Target host processor architecture (-march=native)" ON)

# Module Orchestration
include(cmake/Platform.cmake)
include(cmake/Compiler.cmake)
include(cmake/Dependencies.cmake)

add_subdirectory(src)

if(INK_BUILD_TESTS)
    include(CTest)
    add_subdirectory(tests)
endif()

include(cmake/Install.cmake)