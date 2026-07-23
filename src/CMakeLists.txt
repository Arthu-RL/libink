file(GLOB_RECURSE INK_SOURCES  CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE INK_HEADERS  CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/include/ink/*.h"
    "${PROJECT_SOURCE_DIR}/include/ink/*.hpp"
)

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

target_link_libraries(ink
    PUBLIC
        Threads::Threads
        nlohmann_json::nlohmann_json
)

# Compile options
target_compile_options(ink PRIVATE
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

# forward -pthread so object files are compiled with shared-memory model
if(EMSCRIPTEN)
    target_compile_options(ink PUBLIC -pthread)
    target_link_options(ink  PUBLIC -pthread)
endif()
