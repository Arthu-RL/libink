#ifndef VAC_BASE_HPP
#define VAC_BASE_HPP

/**
 * @file vac_base.hpp
 * @brief Base definitions for VAC library
 *
 * This file contains common macros and basic definitions needed by all VAC components.
 */

#pragma once

/*====================
 * VERSION INFO
 *====================*/
#define VAC_MAJOR_VERSION 0
#define VAC_MINOR_VERSION 1
#define VAC_PATCH_VERSION 0
#define VAC_VERSION ((VAC_MAJOR_VERSION * 10000) + (VAC_MINOR_VERSION * 100) + VAC_PATCH_VERSION)
#define VAC_VERSION_STRING "0.1.0"

/*====================
 * C++ VERSION CHECK
 *====================*/
#ifndef __cplusplus
#error "VAC requires a C++ compiler"
#elif __cplusplus < 201703L
#error "VAC requires C++17 or later"
#endif

/*====================
 * COMPILER DETECTION
 *====================*/
#if defined(__clang__)
#define VAC_COMPILER_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define VAC_COMPILER_GCC 1
#elif defined(_MSC_VER)
#define VAC_COMPILER_MSVC 1
#endif

/*====================
 * PLATFORM DETECTION
 *====================*/
#if defined(_WIN32) || defined(_WIN64)
#define VAC_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
#define VAC_PLATFORM_APPLE 1
#elif defined(__linux__)
#define VAC_PLATFORM_LINUX 1
#elif defined(__unix__)
#define VAC_PLATFORM_UNIX 1
#endif

/*====================
 * BASIC DEFINITIONS
 *====================*/
#define VAC_UNUSED(x) (void)(x)
#define VAC_INLINE inline
#define VAC_NULL nullptr

/* Boolean values */
#define VAC_TRUE 1
#define VAC_FALSE 0

/*====================
 * COMPILER SPECIFIC
 *====================*/
#if defined(VAC_COMPILER_GCC) || defined(VAC_COMPILER_CLANG)
#define VAC_DEPRECATED [[deprecated]]
#define VAC_FORCEINLINE __attribute__((always_inline)) VAC_INLINE
#define VAC_NOINLINE __attribute__((noinline))
#define VAC_NORETURN [[noreturn]]
#define VAC_PACKED __attribute__((packed))
#define VAC_PRINTF_LIKE(fmt_pos, args_pos) __attribute__((format(printf, fmt_pos, args_pos)))
#define VAC_LIKELY(x) __builtin_expect(!!(x), 1)
#define VAC_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define VAC_ALIGN(x) __attribute__((aligned(x)))
#elif defined(VAC_COMPILER_MSVC)
#define VAC_DEPRECATED [[deprecated]]
#define VAC_FORCEINLINE __forceinline
#define VAC_NOINLINE __declspec(noinline)
#define VAC_NORETURN [[noreturn]]
#define VAC_PACKED
#define VAC_PRINTF_LIKE(fmt_pos, args_pos)
#define VAC_LIKELY(x) (x)
#define VAC_UNLIKELY(x) (x)
#define VAC_ALIGN(x) __declspec(align(x))
#else
#define VAC_DEPRECATED [[deprecated]]
#define VAC_FORCEINLINE VAC_INLINE
#define VAC_NOINLINE
#define VAC_NORETURN [[noreturn]]
#define VAC_PACKED
#define VAC_PRINTF_LIKE(fmt_pos, args_pos)
#define VAC_LIKELY(x) (x)
#define VAC_UNLIKELY(x) (x)
#define VAC_ALIGN(x)
#endif

/* Export/Import symbols */
#if defined(VAC_SHARED) && defined(VAC_PLATFORM_WINDOWS)
#ifdef VAC_EXPORT
#define VAC_API __declspec(dllexport)
#else
#define VAC_API __declspec(dllimport)
#endif
#elif defined(VAC_SHARED)
#ifdef VAC_EXPORT
#define VAC_API __attribute__((visibility("default")))
#else
#define VAC_API
#endif
#else
#define VAC_API
#endif

/*====================
 * TYPE DEFINITIONS
 *====================*/
#include <cstdint>
#include <cstddef>
#include <cstring>

/* Return codes */
enum class vac_result_t {
    SUCCESS = 0,
    ERROR_GENERIC = -1,
    ERROR_INVALID_PARAM = -2,
    ERROR_OUT_OF_MEMORY = -3,
    ERROR_NOT_IMPLEMENTED = -4,
    ERROR_NOT_SUPPORTED = -5,
    ERROR_IO = -6
};

using vac_i8 = int8_t;
using vac_i16 = int16_t;
using vac_i32 = int32_t;
using vac_i64 = int64_t;

using vac_u8 = uint8_t;
using vac_u16 = uint16_t;
using vac_u32 = uint32_t;
using vac_u64 = uint64_t;

using vac_f32 = float;
using vac_f64 = double;

using vac_bool = bool;

using vac_size = size_t;
using vac_ptrdiff = ptrdiff_t;

using vac_handle = void*;

/*====================
 * UTILITY MACROS
 *====================*/
/* Min/Max */
#define VAC_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define VAC_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define VAC_CLAMP(x, min, max) (VAC_MIN(VAC_MAX((x), (min)), (max)))

/* Array operations */
#define VAC_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define VAC_ARRAY_EMPTY(arr) ((VAC_ARRAY_SIZE(arr)) == 0)

/* Flag operations */
#define VAC_FLAG_SET(flags, flag) ((flags) |= (flag))
#define VAC_FLAG_CLEAR(flags, flag) ((flags) &= ~(flag))
#define VAC_FLAG_TOGGLE(flags, flag) ((flags) ^= (flag))
#define VAC_FLAG_CHECK(flags, flag) (((flags) & (flag)) == (flag))

/*====================
 * MEMORY OPERATIONS
 *====================*/
#define VAC_ZERO_MEMORY(ptr, size) std::memset((ptr), 0, (size))
#define VAC_ALIGN_SIZE(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))

/*====================
 * LIBRARY CONFIG
 *====================*/
/* Memory allocation/deallocation functions */
#ifndef VAC_MALLOC
#include <cstdlib>
#define VAC_MALLOC(size) std::malloc(size)
#define VAC_FREE(ptr) std::free(ptr)
#define VAC_REALLOC(ptr, size) std::realloc(ptr, size)
#define VAC_CALLOC(count, size) std::calloc(count, size)
#endif

/* String functions */
#ifndef VAC_STRLEN
#include <cstring>
#define VAC_STRLEN(str) std::strlen(str)
#define VAC_STRCMP(a, b) std::strcmp(a, b)
#define VAC_STRNCMP(a, b, n) std::strncmp(a, b, n)
#define VAC_STRCPY(dst, src) std::strcpy(dst, src)
#define VAC_STRNCPY(dst, src, n) std::strncpy(dst, src, n)
#endif

#endif // VAC_BASE_H
