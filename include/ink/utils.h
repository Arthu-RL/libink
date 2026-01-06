#ifndef UTILS_H
#define UTILS_H

#pragma once

#include <string>

namespace ink {

namespace utils {

std::string exec_command(const std::string& cmd);

int cto_int(char c) noexcept;

size_t string_int(std::string_view s) noexcept;

}

}

#endif // UTILS_H
