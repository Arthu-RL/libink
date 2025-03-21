#include "../include/ink/utils.h"

#include <memory>
#include <vector>
#include "../include/ink/InkException.h"

namespace ink {

namespace utils {

std::string exec_command(const std::string& cmd)
{
    // Use unique_ptr with custom deleter for RAII on the pipe
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    INK_THROW_IF(!pipe, "Command execution failed");

    std::string result;
    result.reserve(16384); // 16KB initial reservation

    std::vector<char> buffer(8192);

    // Read directly into our buffer
    size_t bytesRead;
    while ((bytesRead = fread(buffer.data(), 1, buffer.size(), pipe.get())) > 0) {
        // Append only the bytes actually read
        result.append(buffer.data(), bytesRead);

        if (bytesRead < buffer.size())
            break;
    }

    return result;
}

}

}
