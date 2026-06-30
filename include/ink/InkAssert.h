#ifndef INKASSERT_H
#define INKASSERT_H

#include <source_location>
#include <utility>

#include "ink/Inkogger.h"

#if defined(INK_CONFIG_DIST)
#define INK_DISABLE_ASSERTS
#endif

[[noreturn]] inline INK_API void ReportAssertionFailure(
    const char* expression,
    const std::string& message,
    std::source_location location = std::source_location::current())
{
    const char* filename = location.file_name();

    INK_LOG << ink::LoggerColors::BOLD << ink::LoggerColors::RED << "ASSERTION FAILED: "
            << expression << ink::LoggerColors::RESET;

    if (!message.empty()) {
        INK_LOG << ink::LoggerColors::BOLD << ink::LoggerColors::RED << "Message: "
                << message << ink::LoggerColors::RESET;
    }

    INK_LOG << ink::LoggerColors::BOLD << ink::LoggerColors::RED << "Location: "
            << filename << ':' << location.line() << ink::LoggerColors::RESET;

#if defined(_MSC_VER)
    __debugbreak();
#elif defined(__EMSCRIPTEN__)
    __builtin_trap();
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__x86_64__) || defined(__i386__)
    __asm__ volatile("int $3");
#else
    raise(SIGTRAP);
#endif
#else
    raise(SIGABRT);
#endif

    std::unreachable();
}

#ifdef INK_DISABLE_ASSERTS
#define INK_ASSERT(condition) do { (void)sizeof(condition); } while(false)
#define INK_ASSERT_MSG(condition, message) do { (void)sizeof(condition); } while(false)
#else
#define INK_ASSERT(condition) \
do { \
        if (!(condition)) { \
            ReportAssertionFailure(#condition, "Condition Not Satisfied!"); \
    } \
} while (false)

#define INK_ASSERT_MSG(condition, message) \
    do { \
        if (!(condition)) { \
            ReportAssertionFailure(#condition, message); \
    } \
} while (false)
#endif

#endif // INKASSERT_H
