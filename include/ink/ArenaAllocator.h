#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <stddef.h>
#include <string.h>

#include "ink/ink_base.hpp"

#if defined(INK_PLATFORM_WINDOWS)
// mmap/munmap have no Windows equivalent; arena_new_block/arena_destroy
// (ArenaAllocator.cpp) reserve+commit anonymous pages via VirtualAlloc/
// VirtualFree instead. NOMINMAX/WIN32_LEAN_AND_MEAN are already set
// globally (see cmake/Platform.cmake) but are repeated here so this header
// is safe to include standalone.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif
#endif

namespace ink {

class InkedArena {
public:
    struct ArenaBlock {
        u8* memory;
        size_t size;
        size_t offset;
        ArenaBlock* next;
    };

    struct Arena {
        ArenaBlock* head;
        size_t block_size;
    };

    // Static helper to create blocks
    static ArenaBlock* arena_new_block(size_t size);

    // Init/Destroy
    void arena_init(Arena* a, size_t block_size);
    void arena_reset(Arena* a);
    void arena_destroy(Arena* a);

    inline static void* arena_alloc_block(ArenaBlock* b, size_t size, size_t align)
    {
        const u64 base = reinterpret_cast<u64>(b->memory);
        const u64 current_ptr = base + static_cast<u64>(b->offset);
        const u64 dest_ptr = INK_ALIGN_SIZE(current_ptr, align);

        const size_t new_offset = static_cast<size_t>(dest_ptr - base) + size;

        if (new_offset > b->size)
        {
            return nullptr;
        }

        b->offset = new_offset;
        return reinterpret_cast<void*>(dest_ptr);
    }

    // Main allocation function
    inline void* arena_alloc(Arena* a, size_t size, size_t align)
    {
        // Fast Path tring to alloc in current head
        ArenaBlock* b = a->head;

        if (b)
        {
            void* mem = arena_alloc_block(b, size, align);
            if (mem) return mem;
        }

        return arena_alloc_hard(a, size, align);
    }

private:
    void* arena_alloc_hard(Arena* a, size_t size, size_t align);
};

}

#endif
