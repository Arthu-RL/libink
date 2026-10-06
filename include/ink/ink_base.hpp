#ifndef ink_base_HPP
#define ink_base_HPP

#ifndef __cplusplus
#error "INK requires a C++ compiler"
#elif __cplusplus < 202100L
#error "INK requires C++23 or later (compile with -std=c++23)"
#endif

#define INK_STR_HELPER(x) #x
#define INK_STR(x) INK_STR_HELPER(x)

#define INK_CONCAT_HELPER(a, b) a##b
#define INK_CONCAT(a, b) INK_CONCAT_HELPER(a, b)

#define INK_UNUSED(x) (void)(x)

#if defined(_WIN32) || defined(_WIN64)
#define INK_PLATFORM_WINDOWS 1
#elif defined(__APPLE__) && defined(__MACH__)
#define INK_PLATFORM_APPLE 1
#elif defined(__ANDROID__)
#define INK_PLATFORM_ANDROID 1
#elif defined(__linux__)
#define INK_PLATFORM_LINUX 1
#elif defined(__unix__)
#define INK_PLATFORM_UNIX 1
#endif

#if defined(INK_SHARED) && defined(INK_PLATFORM_WINDOWS)
#ifdef INK_EXPORT
#define INK_API __declspec(dllexport)
#else
#define INK_API __declspec(dllimport)
#endif
#elif defined(INK_SHARED) && defined(INK_EXPORT)
#define INK_API __attribute__((visibility("default")))
#else
#define INK_API
#endif

/** GCC/Clang predefine the exact types <cstdint> uses; MSVC's <cstdint> uses these. */
#if defined(__INT8_TYPE__)
using i8 = __INT8_TYPE__;
using i16 = __INT16_TYPE__;
using i32 = __INT32_TYPE__;
using i64 = __INT64_TYPE__;
using u8 = __UINT8_TYPE__;
using u16 = __UINT16_TYPE__;
using u32 = __UINT32_TYPE__;
using u64 = __UINT64_TYPE__;
#else
using i8 = signed char;
using i16 = short;
using i32 = int;
using i64 = long long;
using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;
#endif

using usize = decltype(sizeof(0));
using isize = decltype(static_cast<char *>(nullptr) - static_cast<char *>(nullptr));

using f32 = float;
using f64 = double;

using ink_h = void *;

namespace ink
{

/** No ERROR_* spellings: <winerror.h> defines them as macros, scoped or not. */
enum class Result : i32
{
    Ok = 0,
    Generic = -1,
    InvalidParam = -2,
    OutOfMemory = -3,
    NotImplemented = -4,
    NotSupported = -5,
    Io = -6
};

/** Rounds @p size up to a power-of-two @p alignment, in the wider of the two types. */
template <typename T, typename A> [[nodiscard]] constexpr auto align_up(T size, A alignment) noexcept
{
    using C = decltype(size + alignment);
    return (static_cast<C>(size) + static_cast<C>(alignment) - 1) & ~(static_cast<C>(alignment) - 1);
}

} // namespace ink

namespace ink::detail
{

template <typename T, usize N> auto array_size_helper(T (&)[N]) -> char (&)[N];

template <typename F> class Deferred
{
  public:
    explicit Deferred(F &&fn) noexcept : _fn(static_cast<F &&>(fn))
    {
    }
    Deferred(const Deferred &) = delete;
    Deferred &operator=(const Deferred &) = delete;
    ~Deferred() noexcept
    {
        _fn();
    }

  private:
    F _fn;
};

struct DeferTag
{
};

template <typename F> Deferred<F> operator+(DeferTag, F &&fn) noexcept
{
    return Deferred<F>(static_cast<F &&>(fn));
}

} // namespace ink::detail

/*====================
 * UTILITY MACROS
 *====================*/
/* Array operations */
/** Fails to compile on a pointer, where a sizeof division would silently return a wrong count. */
#define INK_ARRAY_SIZE(arr) sizeof(::ink::detail::array_size_helper(arr))
#define INK_ARRAY_EMPTY(arr) ((INK_ARRAY_SIZE(arr)) == 0)

/* Flag operations */
#define INK_FLAG_SET(flags, flag) ((flags) |= (flag))
#define INK_FLAG_CLEAR(flags, flag) ((flags) &= ~(flag))
#define INK_FLAG_TOGGLE(flags, flag) ((flags) ^= (flag))
#define INK_FLAG_CHECK(flags, flag) (((flags) & (flag)) == (flag))

/* E is a type and INK_DEFER a declaration: neither can be parenthesized. */
// NOLINTBEGIN(bugprone-macro-parentheses)
/** Bitwise operators for an enum class, so INK_FLAG_* work on it. Expand at namespace scope next to the enum,
 *  qualified for a nested one (inside a class they would become members): ADL finds them there. */
#define INK_ENUM_FLAGS(E)                                                                                              \
    [[nodiscard]] constexpr E operator|(E a, E b) noexcept                                                             \
    {                                                                                                                  \
        return static_cast<E>(static_cast<__underlying_type(E)>(a) | static_cast<__underlying_type(E)>(b));            \
    }                                                                                                                  \
    [[nodiscard]] constexpr E operator&(E a, E b) noexcept                                                             \
    {                                                                                                                  \
        return static_cast<E>(static_cast<__underlying_type(E)>(a) & static_cast<__underlying_type(E)>(b));            \
    }                                                                                                                  \
    [[nodiscard]] constexpr E operator^(E a, E b) noexcept                                                             \
    {                                                                                                                  \
        return static_cast<E>(static_cast<__underlying_type(E)>(a) ^ static_cast<__underlying_type(E)>(b));            \
    }                                                                                                                  \
    [[nodiscard]] constexpr E operator~(E a) noexcept                                                                  \
    {                                                                                                                  \
        return static_cast<E>(~static_cast<__underlying_type(E)>(a));                                                  \
    }                                                                                                                  \
    constexpr E &operator|=(E &a, E b) noexcept                                                                        \
    {                                                                                                                  \
        return a = a | b;                                                                                              \
    }                                                                                                                  \
    constexpr E &operator&=(E &a, E b) noexcept                                                                        \
    {                                                                                                                  \
        return a = a & b;                                                                                              \
    }                                                                                                                  \
    constexpr E &operator^=(E &a, E b) noexcept                                                                        \
    {                                                                                                                  \
        return a = a ^ b;                                                                                              \
    }

/* Scope exit */
#define INK_DEFER auto INK_CONCAT(ink_defer_, __COUNTER__) = ::ink::detail::DeferTag{} + [&]() noexcept -> void
// NOLINTEND(bugprone-macro-parentheses)

/*====================
 * MEMORY OPERATIONS
 *====================*/
#define INK_KIB_TO_BYTES(n) (static_cast<u64>(n) << 10)
#define INK_MIB_TO_BYTES(n) (static_cast<u64>(n) << 20)
#define INK_GIB_TO_BYTES(n) (static_cast<u64>(n) << 30)

#endif // ink_base_HPP
