#include "ink/ArenaAllocator.h"

namespace ink {

InkedArena::ArenaBlock* InkedArena::arena_new_block(size_t size)
{
    // Added MAP_POPULATE to force physical memory allocation immediately.
    // This prevents "Page Faults" when you first access the memory.
    void* raw_mem = mmap(
        NULL,
        sizeof(ArenaBlock) + size,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        -1, 0
        );

    if (raw_mem == MAP_FAILED) return nullptr;

    ArenaBlock* block = static_cast<ArenaBlock*>(raw_mem);

    // (block + 1) moves the pointer by sizeof(ArenaBlock) bytes
    block->memory = reinterpret_cast<u8*>(block + 1);
    block->size = size;
    block->offset = 0;
    block->next = nullptr;

    return block;
}

void InkedArena::arena_init(Arena* a, size_t block_size)
{
    a->block_size = block_size;
    a->head = arena_new_block(block_size);
}

void* InkedArena::arena_alloc_hard(Arena* a, size_t size, size_t align)
{
    size_t new_size = (size > a->block_size) ? size : a->block_size;

    ArenaBlock* new_block = arena_new_block(new_size);
    if (!new_block) return nullptr;

    // Link new block as the new head (LIFO structure)
    new_block->next = a->head;
    a->head = new_block;

    return arena_alloc_block(new_block, size, align);
}

void InkedArena::arena_reset(Arena* a)
{
    for (ArenaBlock* b = a->head; b; b = b->next)
    {
        b->offset = 0;
    }
}

void InkedArena::arena_destroy(Arena* a)
{
    ArenaBlock* b = a->head;
    while (b)
    {
        ArenaBlock* next = b->next;
        munmap(b, sizeof(ArenaBlock) + b->size);
        b = next;
    }
    a->head = nullptr;
}

}
