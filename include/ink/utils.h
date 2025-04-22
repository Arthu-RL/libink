#ifndef UTILS_H
#define UTILS_H

#pragma once

#include <string>

#define MAX_CHUNKS 4096

namespace ink {

namespace utils {

std::string exec_command(const std::string& cmd);

int cto_int(char c) noexcept;

}

}

#endif // UTILS_H
