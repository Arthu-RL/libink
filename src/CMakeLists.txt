file(GLOB_RECURSE INK_ALL_SOURCES CONFIGURE_DEPENDS "*.cpp")
set(INK_THREADING_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/ThreadPool.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/WorkerThread.cpp"
)
set(INK_SOURCES ${INK_ALL_SOURCES})
list(REMOVE_ITEM INK_SOURCES ${INK_THREADING_SOURCES})

file(GLOB_RECURSE INK_HEADERS  CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/ink/*.h"
    "${PROJECT_SOURCE_DIR}/include/ink/*.hpp"
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

# ---- ink::ink (core) ----
add_library(ink STATIC)
add_library(ink::ink ALIAS ink)

target_sources(ink
    PRIVATE
        ${INK_SOURCES}
    PUBLIC
        FILE_SET public_headers
        TYPE HEADERS
        BASE_DIRS ${PROJECT_SOURCE_DIR}/include
        FILES ${INK_HEADERS}
)

target_include_directories(ink PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(ink PUBLIC nlohmann_json::nlohmann_json)

if(NOT EMSCRIPTEN)
    target_link_libraries(ink PUBLIC Threads::Threads)
endif()

# Inkogger routes console output through __android_log_print on Android,
# which lives in the NDK's liblog.
if(ANDROID)
    target_link_libraries(ink PUBLIC log)
endif()

ink_apply_common_compile_options(ink)

add_library(threading STATIC)
add_library(ink::threading ALIAS threading)

target_sources(threading PRIVATE ${INK_THREADING_SOURCES})

target_link_libraries(threading PUBLIC ink Threads::Threads)

# forward -pthread so object files are compiled with shared-memory model.
# See the file-level comment above for why this stays off of ink::ink.
# (Threads::Threads already carries -pthread as an interface option on
# Emscripten, but the manual flags are kept here too so this target doesn't
# silently depend on that CMake/toolchain implementation detail.)
if(EMSCRIPTEN)
    target_compile_options(threading PUBLIC -pthread)
    target_link_options(threading PUBLIC -pthread)
endif()

ink_apply_common_compile_options(threading)
