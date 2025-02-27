#pragma once

#define VAC_MAJOR_VERSION 0
#define VAC_MINOR_VERSION 1
#define VAC_PATCH_VERSION 0

#define VAC_VERSION ((VAC_MAJOR_VERSION * 10000) + (VAC_MINOR_VERSION * 100) + VAC_PATCH_VERSION)

#ifndef __cplusplus
    #error "VAC requires a C++ compiler"
#elif __cplusplus < 201703L
    #error "VAC requires C++17 or later"
#endif

// Core Modules
#include <vac/threadpool.h>

// Some Features
// #ifdef VAC_ENABVLE_LOGGING
//     #include <vac/logging.h>
// #endif

// Helper Macros
#define VAC_UNUSED(x) (void)(x)
#define VAC_INLINE inline

#if defined(VAC_SHARED) && defined(_WIN32)
    #ifdef VAC_EXPORT
        #define VAC_API __declspec(dllexport)
    #else
        #define VAC_API __declspec(dllimport)
    #endif
#elif defined(VAC_SHARED)
    #define VAC_API
__attribute__((visibility("default")))
#else
    #define VAC_API
#endif
