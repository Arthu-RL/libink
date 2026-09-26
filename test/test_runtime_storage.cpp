#include "ink/ArenaResource.h"
#include "ink/ParallelProcessor.h"
#include <array>
#include <atomic>
#include <limits>
#include <memory_resource>
#include <stdexcept>

int main()
{
    ink::ParallelProcessor executor(4);
    std::array<int, 103> values{};
    executor.run(values.size(),
                 [&](std::size_t i)
                 {
                     values[i] = static_cast<int>(i);
                 });
    for (std::size_t i = 0; i < values.size(); ++i)
        if (values[i] != static_cast<int>(i))
            return 1;
    std::atomic<int> nested{0};
    executor.run(4,
                 [&](std::size_t)
                 {
                     executor.run(3,
                                  [&](std::size_t)
                                  {
                                      ++nested;
                                  });
                 });
    if (nested != 12)
        return 2;
    bool caught = false;
    try
    {
        executor.run(4,
                     [](std::size_t i)
                     {
                         if (i == 2)
                             throw std::runtime_error("failure");
                     });
    }
    catch (const std::runtime_error &)
    {
        caught = true;
    }
    if (!caught)
        return 3;
    executor.run(3,
                 [](std::size_t)
                 {
                 });

    ink::ArenaResource arena(128);
    bool allocationFailed = false;
    try
    {
        (void)arena.allocate(std::numeric_limits<std::size_t>::max(), 64);
    }
    catch (const std::bad_alloc &)
    {
        allocationFailed = true;
    }
    if (!allocationFailed)
        return 6;
    void *first;
    {
        ink::ArenaResource::Scope outer(arena);
        first = arena.allocate(64, 64);
        ink::ArenaResource::Scope inner(arena);
        std::pmr::vector<int> values{&arena};
        values.assign(10000, 42);
        if (values.back() != 42)
            return 4;
    }
    {
        ink::ArenaResource::Scope outer(arena);
        if (arena.allocate(64, 64) != first)
            return 5;
    }
}
