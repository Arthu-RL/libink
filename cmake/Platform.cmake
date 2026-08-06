if(EMSCRIPTEN)
    message(STATUS "[ink] Target platform: WebAssembly (Emscripten ${EMSCRIPTEN_VERSION})")
    set(INK_NATIVE_OPTIMIZE OFF CACHE BOOL "" FORCE)
    set(INK_ENABLE_LTO     OFF CACHE BOOL "" FORCE)
    set(INK_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
endif()

if(ANDROID)
    message(STATUS "[ink] Target platform: Android  ABI=${ANDROID_ABI}  API=${ANDROID_PLATFORM}")
    set(INK_NATIVE_OPTIMIZE OFF CACHE BOOL "" FORCE)
    set(INK_BUILD_TESTS     OFF CACHE BOOL "" FORCE)

    # NDK provides its own libc++ and pthreads; nothing extra required here.
    # Thumb-2 interworking is enabled by default for arm64-v8a / x86_64.
    # For armeabi-v7a add hardware FP:
    if(ANDROID_ABI STREQUAL "armeabi-v7a")
        add_compile_options(-mfpu=neon -mfloat-abi=softfp)
    endif()
endif()

if(WIN32 AND NOT EMSCRIPTEN)
    message(STATUS "[ink] Target platform: Windows  Compiler=${CMAKE_CXX_COMPILER_ID}")

    # <windows.h>'s min/max macros collide with std::min/std::max and this
    # library's own INK_MIN/INK_MAX (ink_base.hpp); WIN32_LEAN_AND_MEAN keeps
    # the winsock/GDI surface (unused here) out of the build entirely.
    # Applies to every target in the tree, including consumers that pull in
    # <windows.h> themselves.
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

    if(MSVC)
        # /EHsc: standard C++ exception unwinding (off by default under
        # cl.exe; the library throws std::bad_alloc/std::invalid_argument).
        # /utf-8: source and execution charset, matching GCC/Clang defaults.
        add_compile_options(/EHsc /utf-8)
    endif()
endif()
