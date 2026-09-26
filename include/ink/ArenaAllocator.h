#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <bit>
#include <cstddef>
#include <cstdint>

#include "ink/ink_base.hpp"

namespace ink
{

/** Single-threaded raw storage; reset invalidates allocations without destroying objects. */
class InkedArena
{
  public:
    struct ArenaBlock
    {
        u8 *memory;
        usize size;
        usize offset;
        ArenaBlock *next;
    };

    struct Arena
    {
        Arena() noexcept = default;
        explicit Arena(usize block_size) noexcept;
        ~Arena();

        Arena(const Arena &) = delete;
        Arena &operator=(const Arena &) = delete;
        Arena(Arena &&other) noexcept;
        Arena &operator=(Arena &&other) noexcept;

        // Do not edit a live arena's block chain or offsets.
        ArenaBlock *head = nullptr;
        usize block_size = 0;

      private:
        friend class InkedArena;
        ArenaBlock *_current = nullptr;
    };

    /** A standalone block must be adopted by an empty Arena's head for cleanup. */
    [[nodiscard]] static ArenaBlock *arena_new_block(usize size) noexcept;

    void arena_init(Arena *a, usize block_size) noexcept;
    /** O(1); retains blocks and rewinds subsequent blocks lazily as they are reused. */
    void arena_reset(Arena *a) noexcept;
    void arena_destroy(Arena *a) noexcept;

    [[nodiscard]] static void *arena_alloc_block(ArenaBlock *b, usize size, usize align) noexcept
    {
        if (size == 0 || !std::has_single_bit(align) || b->offset > b->size)
        {
            return nullptr;
        }

        u8 *current = b->memory + b->offset;
        const auto address = reinterpret_cast<std::uintptr_t>(current);
        const usize padding = (std::uintptr_t{0} - address) & (align - 1);
        const usize available = b->size - b->offset;

        // Subtract before adding so an oversized request cannot wrap into the block.
        if (padding > available || size > available - padding)
        {
            return nullptr;
        }

        b->offset += padding + size;
        return current + padding;
    }

    /** Returns nullptr for zero size, invalid alignment, overflow, or allocation failure. */
    [[nodiscard]] void *arena_alloc(Arena *a, usize size, usize align = alignof(std::max_align_t)) noexcept
    {
        if (ArenaBlock *b = a->_current)
        {
            if (void *mem = arena_alloc_block(b, size, align)) [[likely]]
            {
                return mem;
            }
        }

        return arena_alloc_hard(a, size, align);
    }

  private:
    void *arena_alloc_hard(Arena *a, usize size, usize align) noexcept;
};

} // namespace ink

#endif
