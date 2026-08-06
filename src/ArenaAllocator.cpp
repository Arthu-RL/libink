#include "ink/ArenaAllocator.h"

// mmap/munmap have no Windows equivalent; arena_new_block/arena_destroy
// below reserve+commit anonymous pages via VirtualAlloc/VirtualFree
// instead.
#if defined(INK_PLATFORM_WINDOWS)
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

InkedArena::ArenaBlock* InkedArena::arena_new_block(size_t size)
{
    const size_t total = sizeof(ArenaBlock) + size;

#if defined(INK_PLATFORM_WINDOWS)
    // MEM_COMMIT forces immediate physical backing, mirroring MAP_POPULATE
    // below so first access doesn't take a page fault.
    void* raw_mem = VirtualAlloc(nullptr, total, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (raw_mem == nullptr) 
        return nullptr;
#else
    // Added MAP_POPULATE to force physical memory allocation immediately.
    // This prevents "Page Faults" when you first access the memory.
    void* raw_mem = mmap(
        NULL,
        total,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        -1, 0
        );

    if (raw_mem == MAP_FAILED) 
        return nullptr;
#endif

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
    size_t new_size = (size > a->block_size) ? (size + align) : a->block_size;

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
#if defined(INK_PLATFORM_WINDOWS)
        VirtualFree(b, 0, MEM_RELEASE);
#else
        munmap(b, sizeof(ArenaBlock) + b->size);
#endif
        b = next;
    }
    a->head = nullptr;
}

}
