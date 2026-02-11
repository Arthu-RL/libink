#ifndef INK_ALIGNED_ALLOCATOR_HPP
#define INK_ALIGNED_ALLOCATOR_HPP

#include <cstdlib>
#include <memory>
#include <new>
#include <limits>
#include <type_traits>

namespace ink {

template<typename T, std::size_t Alignment = 32>
class AlignedAllocator {
public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using propagate_on_container_move_assignment = std::true_type;
    using is_always_equal = std::true_type;

    template<class U>
    struct rebind {
        using other = AlignedAllocator<U, Alignment>;
    };

    static_assert((Alignment & (Alignment - 1)) == 0,
                  "Alignment must be power of two");

    static_assert(Alignment >= alignof(void*),
                  "Alignment must be >= pointer alignment");

public:
    constexpr AlignedAllocator() noexcept = default;

    template<class U>
    constexpr AlignedAllocator(const AlignedAllocator<U, Alignment>&) noexcept {}

    // alloc
    [[nodiscard]]
    pointer allocate(size_type n)
    {
        if (n > std::numeric_limits<size_type>::max() / sizeof(T))
            throw std::bad_alloc();

        void* ptr = nullptr;
        const size_type bytes = n * sizeof(T);

#if defined(_MSC_VER)
        ptr = _aligned_malloc(bytes, Alignment);
        if (!ptr) throw std::bad_alloc();
#else
        if (posix_memalign(&ptr, Alignment, bytes) != 0)
            throw std::bad_alloc();
#endif

        return static_cast<pointer>(ptr);
    }

    // dealloc
    void deallocate(pointer p, size_type) noexcept
    {
#if defined(_MSC_VER)
        _aligned_free(p);
#else
        std::free(p);
#endif
    }

    template<class U, class... Args>
    void construct(U* p, Args&&... args)
    {
        ::new ((void*)p) U(std::forward<Args>(args)...);
    }

    template<class U>
    void destroy(U* p)
    {
        p->~U();
    }

    // cmp
    constexpr bool operator==(const AlignedAllocator&) const noexcept { return true; }
    constexpr bool operator!=(const AlignedAllocator&) const noexcept { return false; }
};

} // namespace ink

#endif
