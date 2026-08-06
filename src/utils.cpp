#include "../include/ink/utils.h"

#include <charconv>
#include <chrono>

#if !defined(INK_PLATFORM_WINDOWS)
#include <time.h>
#endif

namespace ink {

namespace utils {

constexpr usize MAX_CHUNKS = 4096;

std::expected<std::string, ink_result_t> exec_command(const std::string& cmd)
{
    // popen/pclose are POSIX; MSVC's CRT exposes the same pipe-a-child-process
    // behavior under the _popen/_pclose spelling instead.
#if defined(INK_PLATFORM_WINDOWS)
    FILE* pipe = _popen(cmd.c_str(), "r");
#else
    FILE* pipe = popen(cmd.data(), "r");
#endif
    if (!pipe) {
        return std::unexpected(ink_result_t::ERROR_IO);
    }

    std::string result;
    result.reserve(1024);

    char buffer[MAX_CHUNKS];

    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, MAX_CHUNKS, pipe)) > 0) {
        result.append(buffer, bytesRead);
    }

#if defined(INK_PLATFORM_WINDOWS)
    _pclose(pipe);
#else
    pclose(pipe);
#endif

    return result;
}

i32 cto_int(char c) noexcept
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    return -1;
}

std::expected<usize, ink_result_t> string_int(std::string_view s) noexcept
{
    usize result = 0;
    const auto parsed = std::from_chars(s.data(), s.data() + s.size(), result);
    if (parsed.ec != std::errc{}) {
        return std::unexpected(ink_result_t::ERROR_INVALID_PARAM);
    }
    return result;
}

u64 nowMillis()
{
#if defined(INK_PLATFORM_WINDOWS)
    // No POSIX CLOCK_MONOTONIC_COARSE on Windows; steady_clock maps to
    // QueryPerformanceCounter, which is monotonic and cheap enough here.
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return static_cast<u64>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
#else
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);

    return static_cast<u64>(ts.tv_sec) * 1000
           + static_cast<u64>(ts.tv_nsec) / 1'000'000;
#endif
}

}

}
