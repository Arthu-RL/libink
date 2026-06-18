#include "../include/ink/utils.h"

#include <time.h>
#include <charconv>

namespace ink {

namespace utils {

constexpr usize MAX_CHUNKS = 4096;

std::expected<std::string, ink_result_t> exec_command(const std::string& cmd)
{
    FILE* pipe = popen(cmd.data(), "r");
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

    pclose(pipe);

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
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);

    return static_cast<u64>(ts.tv_sec) * 1000
           + static_cast<u64>(ts.tv_nsec) / 1'000'000;
}

}

}
