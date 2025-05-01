#ifndef ink_base_HPP
#define ink_base_HPP

/**
 * @file ink_base.hpp
 * @brief Base definitions for INK library
 *
 * This file contains common macros and basic definitions needed by all INK components.
 */

#pragma once

/*====================
 * Good Macros
 *====================*/
// Double stringification
#define INK_STR_HELPER(x) #x
#define INK_STR(x) INK_STR_HELPER(x)

// Token pasting (concatenation)
#define CONCAT(a, b) a##b
#define CONCAT_EXPAND(a, b) CONCAT(a, b)

#define LOCATION __FILE__ ":" INK_STR(__LINE__)

// Compile-time assertions
#define STATIC_ASSERT(cond, msg) typedef char static_assertion_##msg[(cond) ? 1 : -1]

/*====================
 * VERSION INFO
 *====================*/
#define INK_MAJOR_VERSION 1
#define INK_MINOR_VERSION 1
#define INK_PATCH_VERSION 0
#define INK_VERSION ((INK_MAJOR_VERSION * 10000) + (INK_MINOR_VERSION * 100) + INK_PATCH_VERSION)
#define INK_VERSION_STRING_FULL INK_STR(INK_MAJOR_VERSION) "." INK_STR(INK_MINOR_VERSION) "." INK_STR(INK_PATCH_VERSION)

/*====================
 * C++ VERSION CHECK
 *====================*/
#ifndef __cplusplus
#error "INK requires a C++ compiler"
#elif __cplusplus < 201703L
#error "INK requires C++17 or later"
#endif

/*====================
 * COMPILER DETECTION
 *====================*/
#if defined(__clang__)
#define INK_COMPILER_CLANG 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define INK_COMPILER_GCC 1
#elif defined(_MSC_VER)
#define INK_COMPILER_MSVC 1
#endif

/*====================
 * PLATFORM DETECTION
 *====================*/
#if defined(_WIN32) || defined(_WIN64)
#define INK_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
#define INK_PLATFORM_APPLE 1
#elif defined(__linux__)
#define INK_PLATFORM_LINUX 1
#elif defined(__unix__)
#define INK_PLATFORM_UNIX 1
#endif

/*====================
 * BASIC DEFINITIONS
 *====================*/
#define INK_UNUSED(x) (void)(x)
#define INK_INLINE inline
#define INK_NULL nullptr

/* Boolean values */
#define INK_TRUE 1
#define INK_FALSE 0

/*====================
 * COMPILER SPECIFIC
 *====================*/
#if defined(INK_COMPILER_GCC) || defined(INK_COMPILER_CLANG)
#define INK_DEPRECATED [[deprecated]]
#define INK_FORCEINLINE __attribute__((always_inline)) INK_INLINE
#define INK_NOINLINE __attribute__((noinline))
#define INK_NORETURN [[noreturn]]
#define INK_PACKED __attribute__((packed))
#define INK_PRINTF_LIKE(fmt_pos, args_pos) __attribute__((format(printf, fmt_pos, args_pos)))
#define INK_LIKELY(x) __builtin_expect(!!(x), 1)
#define INK_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define INK_ALIGN(x) __attribute__((aligned(x)))
#elif defined(INK_COMPILER_MSVC)
#define INK_DEPRECATED [[deprecated]]
#define INK_FORCEINLINE __forceinline
#define INK_NOINLINE __declspec(noinline)
#define INK_NORETURN [[noreturn]]
#define INK_PACKED
#define INK_PRINTF_LIKE(fmt_pos, args_pos)
#define INK_LIKELY(x) (x)
#define INK_UNLIKELY(x) (x)
#define INK_ALIGN(x) __declspec(align(x))
#else
#define INK_DEPRECATED [[deprecated]]
#define INK_FORCEINLINE INK_INLINE
#define INK_NOINLINE
#define INK_NORETURN [[noreturn]]
#define INK_PACKED
#define INK_PRINTF_LIKE(fmt_pos, args_pos)
#define INK_LIKELY(x) (x)
#define INK_UNLIKELY(x) (x)
#define INK_ALIGN(x)
#endif

/* Export/Import symbols */
#if defined(INK_SHARED) && defined(INK_PLATFORM_WINDOWS)
#ifdef INK_EXPORT
#define INK_API __declspec(dllexport)
#else
#define INK_API __declspec(dllimport)
#endif
#elif defined(INK_SHARED)
#ifdef INK_EXPORT
#define INK_API __attribute__((visibility("default")))
#else
#define INK_API
#endif
#else
#define INK_API
#endif

/*====================
 * TYPE DEFINITIONS
 *====================*/
#include <cstdint>
#include <cstddef>
#include <cstring>

/* Return codes */
enum class ink_result_t {
    SUCCESS = 0,
    ERROR_GENERIC = -1,
    ERROR_INVALID_PARAM = -2,
    ERROR_OUT_OF_MEMORY = -3,
    ERROR_NOT_IMPLEMENTED = -4,
    ERROR_NOT_SUPPORTED = -5,
    ERROR_IO = -6
};

using ink_i8 = int8_t;
using ink_i16 = int16_t;
using ink_i32 = int32_t;
using ink_i64 = int64_t;

using ink_u8 = uint8_t;
using ink_u16 = uint16_t;
using ink_u32 = uint32_t;
using ink_u64 = uint64_t;

using ink_f32 = float;
using ink_f64 = double;

using ink_bool = bool;

using ink_size = size_t;
using ink_ptrdiff = ptrdiff_t;

using ink_handle = void*;

/*====================
 * UTILITY MACROS
 *====================*/
/* Min/Max */
#define INK_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define INK_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define INK_CLAMP(x, min, max) (INK_MIN(INK_MAX((x), (min)), (max)))

/* Array operations */
#define INK_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#define INK_ARRAY_EMPTY(arr) ((INK_ARRAY_SIZE(arr)) == 0)

/* Flag operations */
#define INK_FLAG_SET(flags, flag) ((flags) |= (flag))
#define INK_FLAG_CLEAR(flags, flag) ((flags) &= ~(flag))
#define INK_FLAG_TOGGLE(flags, flag) ((flags) ^= (flag))
#define INK_FLAG_CHECK(flags, flag) (((flags) & (flag)) == (flag))

/*====================
 * MEMORY OPERATIONS
 *====================*/
#define INK_ZERO_MEMORY(ptr, size) std::memset((ptr), 0, (size))
#define INK_ALIGN_SIZE(size, alignment) (((size) + ((alignment) - 1)) & ~((alignment) - 1))

/*====================
 * LIBRARY CONFIG
 *====================*/
/* Memory allocation/deallocation functions */
#ifndef INK_MALLOC
#include <cstdlib>
#define INK_MALLOC(size) std::malloc(size)
#define INK_FREE(ptr) std::free(ptr)
#define INK_REALLOC(ptr, size) std::realloc(ptr, size)
#define INK_CALLOC(count, size) std::calloc(count, size)
#endif

/* String functions */
#ifndef INK_STRLEN
#include <cstring>
#define INK_STRLEN(str) std::strlen(str)
#define INK_STRCMP(a, b) std::strcmp(a, b)
#define INK_STRNCMP(a, b, n) std::strncmp(a, b, n)
#define INK_STRCPY(dst, src) std::strcpy(dst, src)
#define INK_STRNCPY(dst, src, n) std::strncpy(dst, src, n)
#endif

#endif // ink_base_H
