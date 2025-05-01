#ifndef INKASSERT_H
#define INKASSERT_H

#pragma once

#include <cstring>

#include "ink/Inkogger.h"

// Configuration macros
#if defined(INK_CONFIG_DIST)
#define INK_DISABLE_ASSERTS
#endif

inline INK_API void ReportAssertionFailure(const char* expression, const std::string& message, const char* file, int line)
{
    // Extract filename from path
    const char* filename = file;
    const char* lastSlash = strrchr(file, '/');
    const char* lastBackslash = strrchr(file, '\\');

    if (lastSlash != nullptr) {
        filename = lastSlash + 1;
    } else if (lastBackslash != nullptr) {
        filename = lastBackslash + 1;
    }

    const char* msg = message.c_str();

    INK_LOG << ink::LoggerColors::BOLD << ink::LoggerColors::RED << "ASSERTION FAILED: "
            << expression << ink::LoggerColors::RESET;

    if (msg && msg[0] != '\0')
    {
        INK_LOG << ink::LoggerColors::BOLD << ink::LoggerColors::RED << "Message: "
                << msg << ink::LoggerColors::RESET;
    }

    INK_LOG << ink::LoggerColors::BOLD << ink::LoggerColors::RED << "Location: "
            << filename << ':' << line << ink::LoggerColors::RESET;

// Break into the debugger if available
#if defined(_MSC_VER)
    __debugbreak();
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("int $3");
#else
    raise(SIGTRAP);
#endif
#else
    // Fallback using signal
    raise(SIGABRT);
#endif
}

#ifdef INK_DISABLE_ASSERTS
#define INK_ASSERT(condition) do { (void)sizeof(condition); } while(false)
#define INK_ASSERT_MSG(condition, message) do { (void)sizeof(condition); } while(false)
#else
/**
 * Basic assertion that breaks if the condition is false
 */
#define INK_ASSERT(condition) \
do { \
        if (!(condition)) { \
            ReportAssertionFailure(#condition, "Condition Not Satisfied!", __FILE__, __LINE__); \
    } \
} while (false)

/**
 * Assertion with custom message
 */
#define INK_ASSERT_MSG(condition, message) \
    do { \
        if (!(condition)) { \
            ReportAssertionFailure(#condition, message, __FILE__, __LINE__); \
    } \
} while (false)
#endif

#endif // INKASSERT_H
