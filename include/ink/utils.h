#ifndef UTILS_H
#define UTILS_H

#pragma once

#include <string>

#include "../include/ink/ink_base.hpp"

namespace ink {

namespace utils {

std::string exec_command(const std::string& cmd);

i32 cto_int(char c) noexcept;

usize string_int(std::string_view s) noexcept;

u64 nowMillis();

}

}

#endif // UTILS_H
