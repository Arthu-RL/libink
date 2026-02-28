#include "../include/ink/utils.h"

#include <time.h>
#include <charconv>

namespace ink {

namespace utils {

constexpr usize MAX_CHUNKS = 4096;

std::string exec_command(const std::string& cmd)
{
    FILE* pipe = popen(cmd.data(), "r");
    if (!pipe) {
        return "ERROR: PIPE_FAILED";
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

usize string_int(std::string_view s) noexcept
{
    // Use fast string-to-int conversion avoiding exceptions from std::stoul
    size_t result = -1;
    std::from_chars(s.data(), s.data()+s.size(), result);
    return result;
}

u64 nowMillis()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return static_cast<uint64_t>(ts.tv_sec) * 1000
           + static_cast<uint64_t>(ts.tv_nsec) / 1'000'000;
}

}

}
