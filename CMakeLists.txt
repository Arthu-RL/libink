cmake_minimum_required(VERSION 4.3.3)
project(ink
    VERSION 0.5.0
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
include(cmake/PlatformSuffix.cmake)
ink_platform_suffix(INK_PLATFORM_SUFFIX)
message(STATUS "ink: installing as ink_${INK_PLATFORM_SUFFIX}")

include(cmake/Platform.cmake)
include(cmake/Compiler.cmake)
include(cmake/Dependencies.cmake)

# Not a build input: lists docs/ in the IDE project tree.
file(GLOB INK_DOC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/docs/*.md")
add_custom_target(ink_docs SOURCES ${INK_DOC_FILES} README.md CHANGELOG.md)

add_subdirectory(src)

if(INK_BUILD_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()

include(cmake/Install.cmake)
