# INK Library

### A versatile and efficient collection of utilities and implementations

# Overview

INK (Implementations Notorious Kit) is a C++ library designed to provide a wide range of utilities and implementations for modern software development. It aims to deliver high-performance, flexible, and reusable solutions for common tasks, such as multi-threading, data processing, and more. Whether you're building complex systems or just need a fast, efficient helper for your application, INK has you covered.

# Getting Started

## Prerequisites

- C++23 or later

- A modern C++ compiler (GCC, Clang, MSVC, etc.)

# Installation

For Linux: 

```sh
git clone https://github.com/Arthu-RL/libink.git

export LOCAL_PREFIX=/usr/local

cmake -S ./libink -B ./libink/build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${LOCAL_PREFIX} && \
cmake --build ./libink/build --target install
```

# How to use

```cmake
find_package(ink REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC ink)
```

```cpp
#include <ink/ink.hpp>
```

# Acknowledgements

- This library leverages ideas and algorithms from various open-source projects.
