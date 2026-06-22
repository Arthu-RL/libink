#ifndef UTILS_H
#define UTILS_H

#include <expected>
#include <string>
#include <string_view>

#include "ink/ink_base.hpp"

namespace ink {

namespace utils {

std::expected<std::string, ink_result_t> exec_command(const std::string& cmd);

i32 cto_int(char c) noexcept;

std::expected<usize, ink_result_t> string_int(std::string_view s) noexcept;

u64 nowMillis();

}

}

#endif // UTILS_H
