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

################################################################################
# ink library
################################################################################

file(GLOB_RECURSE INK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB_RECURSE INK_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ink/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/ink/*.hpp"
)

# Applies the warning/optimization flags shared by every ink library target.
function(ink_apply_common_compile_options target)
    if(MSVC)
        # cl.exe doesn't understand -Wall/-O3/-march=native (GCC/Clang
        # spellings); INK_NATIVE_OPTIMIZE has no MSVC equivalent (/arch: is
        # ISA-level, not "this host's CPU") so it's intentionally a no-op here.
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:/W4 /Od>
            $<$<CONFIG:Release>:/O2>
        )
    else()
        target_compile_options(${target} PRIVATE
            $<$<CONFIG:Debug>:
                -Wall
                -Wextra
                -Wpedantic
                -g
                -O0
            >
            $<$<CONFIG:Release>:
                -O3
            >
            # -march=native only when explicitly requested and not cross-compiling
            $<$<AND:$<CONFIG:Release>,$<BOOL:${INK_NATIVE_OPTIMIZE}>>:
                -march=native
                -mtune=native
            >
        )
    endif()
endfunction()

add_library(ink STATIC)
add_library(ink::ink ALIAS ink)

#! One flat lib directory holds every platform's build, so the ABI tag is in
#! the filename: libink_linux_x86_64.a, libink_wasm32.a, ...
set_target_properties(ink PROPERTIES OUTPUT_NAME "ink_${INK_PLATFORM_SUFFIX}")

target_sources(ink
    PRIVATE
        ${INK_SOURCES}
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/include
        FILES ${INK_HEADERS}
)

target_include_directories(ink PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ThreadPool/WorkerThread ship in this one target rather than a separate
# ink::threading: ink.hpp includes both headers, so every consumer of the
# aggregate header already depends on them, and splitting the target only meant
# a consumer could compile against a class it could not link.
target_link_libraries(ink
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
)

# Inkogger routes console output through __android_log_print on Android,
# which lives in the NDK's liblog.
if(ANDROID)
    target_link_libraries(ink PUBLIC log)
endif()

# forward -pthread so object files are compiled with shared-memory model.
# On Emscripten this is what makes the module's WebAssembly.Memory shared and
# backed by a SharedArrayBuffer, so every page serving a build of this library
# must send COOP/COEP -- see the WASM notes in the consumers' build scripts.
# (Threads::Threads already carries -pthread as an interface option there, but
# the manual flags are kept so this does not silently depend on that
# CMake/toolchain implementation detail.)
if(EMSCRIPTEN)
    target_compile_options(ink PUBLIC -pthread)
    target_link_options(ink  PUBLIC -pthread)
endif()

ink_apply_common_compile_options(ink)

################################################################################
# Docs
################################################################################

# Not a build input: lists docs/ in the IDE project tree.
file(GLOB INK_DOC_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/docs/*.md")
add_custom_target(ink_docs SOURCES ${INK_DOC_FILES} README.md CHANGELOG.md)

if(INK_BUILD_TESTS)
    include(CTest)
    add_subdirectory(test)
endif()

include(cmake/Install.cmake)
