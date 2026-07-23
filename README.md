# Ink

A modern C++ utility library: allocators, containers, concurrency, and a handful of other things every project ends up rewriting.

## What's inside

- **Memory** — `AlignedAllocator`, `ArenaAllocator`, `ObjectPool`
- **Containers** — `InkedList`, `Queue`, `RingBuffer`, `InkixTree`, `String`
- **Concurrency** — `ThreadPool`, `WorkerThread`, `TimerWheel`
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
