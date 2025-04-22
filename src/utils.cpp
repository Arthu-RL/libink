#include "../include/ink/utils.h"

#include "../include/ink/InkAssert.h"

namespace ink {

namespace utils {

std::string exec_command(const std::string& cmd)
{
    FILE* pipe = popen(cmd.data(), "r");
    INK_ASSERT_MSG(pipe, "PIPE DIDN'T OPEN!");

    std::string result;
    result.reserve(16384);

    char buffer[MAX_CHUNKS];

    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, MAX_CHUNKS, pipe)) > 0) {
        result.append(buffer, 0, bytesRead);
        if (bytesRead < MAX_CHUNKS) break;
    }

    pclose(pipe);

    return result;
}

int cto_int(char c) noexcept
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    } else
    {
        return -1;
    }
}

}

}
