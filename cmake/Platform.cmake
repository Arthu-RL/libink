if(EMSCRIPTEN)
    message(STATUS "[ink] Target platform: WebAssembly (Emscripten ${EMSCRIPTEN_VERSION})")
    set(INK_NATIVE_OPTIMIZE OFF CACHE BOOL "" FORCE)
    set(INK_ENABLE_LTO     OFF CACHE BOOL "" FORCE)
    set(INK_BUILD_TESTS    OFF CACHE BOOL "" FORCE)

    # pthreads are required by ThreadPool / WorkerThread
    # -pthread must appear at both compile and link time for emscripten
    add_compile_options(-pthread)
    add_link_options(-pthread)
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
