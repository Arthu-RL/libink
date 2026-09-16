#include "ink/ArenaAllocator.h"

#include <algorithm>
#include <limits>
#include <memory>
#include <utility>

#if defined(__EMSCRIPTEN__)
#include <cstdlib>
#elif defined(INK_PLATFORM_WINDOWS)
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

InkedArena::Arena::Arena(usize block_size) noexcept
{
    InkedArena{}.arena_init(this, block_size);
}

InkedArena::Arena::~Arena()
{
    InkedArena{}.arena_destroy(this);
}

InkedArena::Arena::Arena(Arena&& other) noexcept
    : head(std::exchange(other.head, nullptr)),
      block_size(std::exchange(other.block_size, 0)),
      _current(std::exchange(other._current, nullptr))
{
}

InkedArena::Arena& InkedArena::Arena::operator=(Arena&& other) noexcept
{
    if (this != &other)
    {
        InkedArena{}.arena_destroy(this);
        head = std::exchange(other.head, nullptr);
        block_size = std::exchange(other.block_size, 0);
        _current = std::exchange(other._current, nullptr);
    }
    return *this;
}

InkedArena::ArenaBlock* InkedArena::arena_new_block(usize size) noexcept
{
    if (size == 0 || size > (std::numeric_limits<usize>::max)() - sizeof(ArenaBlock))
    {
        return nullptr;
    }
    const usize total = sizeof(ArenaBlock) + size;

#if defined(__EMSCRIPTEN__)
    // WASM has linear memory; mmap emulation adds unnecessary page alignment.
    void* raw_mem = std::malloc(total);
    if (raw_mem == nullptr)
        return nullptr;
#elif defined(INK_PLATFORM_WINDOWS)
    void* raw_mem = VirtualAlloc(nullptr, total, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (raw_mem == nullptr) 
        return nullptr;
#else
    // Best-effort prefaulting on Linux moves some first-touch work into block creation.
    void* raw_mem = mmap(
        nullptr,
        total,
        PROT_READ | PROT_WRITE,
        MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE,
        -1, 0
        );

    if (raw_mem == MAP_FAILED) 
        return nullptr;
#endif

    ArenaBlock* block = std::construct_at(static_cast<ArenaBlock*>(raw_mem));
    block->memory = reinterpret_cast<u8*>(block + 1);
    block->size = size;
    block->offset = 0;
    block->next = nullptr;

    return block;
}

void InkedArena::arena_init(Arena* a, usize block_size) noexcept
{
    arena_destroy(a);
    a->block_size = block_size;
    a->head = arena_new_block(block_size);
    a->_current = a->head;
}

void* InkedArena::arena_alloc_hard(Arena* a, usize size, usize align) noexcept
{
    if (size == 0 || !std::has_single_bit(align)) return nullptr;
    const usize max_size = (std::numeric_limits<usize>::max)() - sizeof(ArenaBlock);
    if (align - 1 > max_size || size > max_size - (align - 1)) return nullptr;

    ArenaBlock* b = a->_current ? a->_current->next : a->head;
    for (; b; b = b->next)
    {
        // Every retained block is visited at most once between resets.
        b->offset = 0;
        a->_current = b;
        if (void* mem = arena_alloc_block(b, size, align)) return mem;
    }

    const usize new_size = (std::max)(size + (align - 1), a->block_size);
    ArenaBlock* new_block = arena_new_block(new_size);
    if (!new_block) return nullptr;

    if (a->_current)
        a->_current->next = new_block;
    else
        a->head = new_block;
    a->_current = new_block;

    return arena_alloc_block(new_block, size, align);
}

void InkedArena::arena_reset(Arena* a) noexcept
{
    a->_current = a->head;
    if (a->_current) a->_current->offset = 0;
}

void InkedArena::arena_destroy(Arena* a) noexcept
{
    ArenaBlock* b = a->head;
    while (b)
    {
        ArenaBlock* next = b->next;
#if defined(__EMSCRIPTEN__)
        std::free(b);
#elif defined(INK_PLATFORM_WINDOWS)
        VirtualFree(b, 0, MEM_RELEASE);
#else
        munmap(b, sizeof(ArenaBlock) + b->size);
#endif
        b = next;
    }
    a->head = nullptr;
    a->_current = nullptr;
}

}
