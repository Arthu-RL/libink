#ifndef INK_MOVE_ONLY_FUNCTION_H
#define INK_MOVE_ONLY_FUNCTION_H

#include <functional>

namespace ink
{

/** libc++ (NDK, Emscripten) still lacks std::move_only_function. */
#if defined(__cpp_lib_move_only_function)
template <typename Sig> using move_only_function = std::move_only_function<Sig>;
#else
template <typename Sig> using move_only_function = std::function<Sig>;
#endif

} // namespace ink

#endif
