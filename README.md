# Ink

A modern C++ utility library: allocators, containers, concurrency, and a handful of other things every project ends up rewriting.

## What's inside

- **Memory** — `AlignedAllocator`, `ArenaAllocator`, `ArenaResource`, `ObjectPool`
- **Containers** — `InkedList`, `Queue`, `RingBuffer`, `InkixTree`, `String`
- **Concurrency** — `ThreadPool`, `ParallelProcessor`, `WorkerThread`, `TimerWheel`
- **JSON** — `EnhancedJson` and utilities
- **Misc** — `ArgParser`, `Inkogger` (logging), `InkOtp`, `InkAssert`, `LastWish`, general `utils`

## Prerequisites

- C++23 or later
- A modern C++ compiler (GCC, Clang, MSVC, etc.)

## Installation

```sh
git clone https://github.com/Arthu-RL/libink.git

export LOCAL_PREFIX=/usr/local

cmake -S ./libink -B ./libink/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${LOCAL_PREFIX} && \
cmake --build ./libink/build --target install
```

## Usage

```cmake
find_package(ink REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ink)
```

```cpp
#include <ink/ink.hpp>
```

## Acknowledgements

This library leverages ideas and algorithms from various open-source projects.

## Scoped scratch and synchronous jobs

| API | Contract |
|---|---|
| `ink::ArenaResource` | PMR storage over `InkedArena`; default blocks are 64 KiB. A `Scope` resets only when the outermost scope exits. Destroy containers first. Resources are thread-confined; allocation failure throws `std::bad_alloc`. |
| `ink::ParallelProcessor` | Constructor concurrency includes the calling thread. `run(count, body)` joins every dispatched worker before returning/rethrowing. Parallel callers serialize; recursion on the same processor runs inline. Keep the processor alive until all callers return. |

Scratch blocks retain their high-water allocation until resource destruction.
The processor creates workers once; dispatch creates no task/future storage.
A caller's large `std::function` capture can still allocate.
