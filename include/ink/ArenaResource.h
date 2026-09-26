#pragma once

#include "ink/ArenaAllocator.h"
#include <memory_resource>
#include <new>

namespace ink
{

/**
 * PMR adapter over InkedArena: lets standard containers draw scratch storage
 * from an arena and release it all at once.
 *
 * Deallocation is a no-op, so a container that reallocates leaves its old
 * blocks behind until the outermost Scope ends; reserve up front for
 * anything that grows. Blocks are kept across resets, so a resource that
 * has served its largest workload allocates nothing thereafter.
 *
 * Not thread-safe: one resource per thread.
 */
class ArenaResource final : public std::pmr::memory_resource
{
  public:
    /** @param blockSize Bytes per arena block; a larger request gets its own block. */
    explicit ArenaResource(usize blockSize = usize{64} * 1024) noexcept : _arena(blockSize)
    {
    }

    /**
     * RAII reset. Scopes nest; only the outermost one rewinds the arena, so a
     * callee can take a scope without pulling storage out from under its
     * caller. Every PMR container using the resource must be destroyed before
     * the outermost scope ends: reset does not run destructors.
     */
    class Scope
    {
      public:
        explicit Scope(ArenaResource &resource) noexcept : _resource(resource)
        {
            ++_resource._depth;
        }
        ~Scope()
        {
            if (--_resource._depth == 0)
                InkedArena{}.arena_reset(&_resource._arena);
        }
        Scope(const Scope &) = delete;
        Scope &operator=(const Scope &) = delete;

      private:
        ArenaResource &_resource;
    };

  private:
    /** Throws std::bad_alloc where the arena returns nullptr, as PMR requires. */
    void *do_allocate(std::size_t bytes, std::size_t alignment) override
    {
        if (void *p = InkedArena{}.arena_alloc(&_arena, bytes ? bytes : 1, alignment))
            return p;
        throw std::bad_alloc();
    }
    /** Nothing to do: storage returns with the outermost Scope. */
    void do_deallocate(void *, std::size_t, std::size_t) override
    {
    }
    /** Identity: memory from one arena cannot be handed back to another. */
    bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
    {
        return this == &other;
    }

    InkedArena::Arena _arena;
    /** Open Scope count; the reset fires when it returns to zero. */
    usize _depth = 0;
};

} // namespace ink
