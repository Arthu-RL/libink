#ifndef ARENA_ALLOCATOR_H
#define ARENA_ALLOCATOR_H

#include <sys/mman.h>
#include <stddef.h>
#include <string.h>

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_POPULATE
#define MAP_POPULATE 0
#endif

#include "ink/ink_base.hpp"

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
        u64 current_ptr = (u64)b->memory + (u64)b->offset;

        u64 offset = (u64)b->memory;
        u64 dest_ptr = INK_ALIGN_SIZE(current_ptr, align);

        size_t new_offset = (dest_ptr - offset) + size;

        if (new_offset > b->size)
        {
            return nullptr;
        }

        b->offset = new_offset;
        return (void*)dest_ptr;
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
