# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.2.0]

### Added

- **Windows support**: libink now builds natively on Windows (MSVC, CI
  verified on `windows-latest`/VS 2022). `InkedArena` (`ArenaAllocator`)
  now backs its blocks with `VirtualAlloc`/`VirtualFree` instead of
  POSIX `mmap`/`munmap`, `utils::exec_command` uses `_popen`/`_pclose`,
  and `utils::nowMillis` uses `std::chrono::steady_clock` in place of
  `clock_gettime(CLOCK_MONOTONIC_COARSE, ...)`, which has no Windows
  equivalent. `cmake/Platform.cmake` sets `NOMINMAX`/`WIN32_LEAN_AND_MEAN`
  and `/EHsc`/`/utf-8`/`/Zc:__cplusplus` for MSVC targets -- the last
  because cl.exe pins `__cplusplus` to `199711L` regardless of the active
  `/std:` flag unless that switch is passed, which otherwise trips
  `ink_base.hpp`'s C++23 `#error` guard. `<windows.h>` (needed for
  `VirtualAlloc`/`VirtualFree`) is scoped to `ArenaAllocator.cpp` rather
  than its public header, since it leaks macros -- `ERROR` in particular --
  into every translation unit that includes it, which collided with
  `LogLevel::ERROR` in `Inkogger.h` for any consumer of `ink.hpp`. The
  Release workflow now packages a `windows-x86_64` archive (via
  vcpkg-installed `nlohmann_json`) alongside the existing Linux/Android/
  WebAssembly artifacts.

### Changed

- **Build system**: split the CMake package into two targets --
  `ink::ink` (logging, JSON, containers, everything except real OS
  threads) and `ink::threading` (`ThreadPool`/`WorkerThread`, opt-in).
  On Emscripten, `-pthread` switches the whole WASM module to a shared,
  growable `WebAssembly.Memory` backed by `SharedArrayBuffer`, which only
  instantiates on a cross-origin-isolated page (COOP/COEP headers on
  every response); consumers that never touch `ThreadPool`/`WorkerThread`
  are no longer forced into that deployment requirement just for linking
  `ink`. `-pthread`/`Threads::Threads` now apply only to `ink::threading`.
  Downstream consumers that use `ThreadPool` or `WorkerThread` must add
  `ink::threading` to their `target_link_libraries`.

### Fixed

- **Install rules**: `cmake/Install.cmake` now installs the new
  `threading` target alongside `ink`, so `find_package(ink)` consumers
  can link `ink::threading`.

## [0.1.0]

First documented release. libink is a C++23 core utility and algorithm
library targeting Linux, WebAssembly (Emscripten), and Android (NDK).

### Added

- **Core platform layer** (`ink_base.hpp`): compiler/platform detection
  macros, fixed-width type aliases (`i8`..`u64`, `f32`/`f64`), `INK_API`
  export/import handling, `move_only_function` alias, and general utility
  macros (`INK_MIN`/`MAX`/`CLAMP`, alignment/array/flag helpers).
- **`Inkogger` / `LogManager`**: leveled, thread-safe logging system
  (`OFF`..`TRACE`) with a stream-style (`INK_INFO << ...`) and
  printf-style macro API, ANSI-colored console output, optional file
  sink, and per-name logger registry with a fast-path cached core logger.
  Console output routes through `__android_log_print` on Android and
  through a lock-free `O_APPEND` file descriptor on POSIX targets when
  logging to a file.
- **`InkAssert`**: `INK_ASSERT` / `INK_ASSERT_MSG` runtime assertions with
  `std::source_location` reporting and a platform-appropriate trap
  (`int3`/`SIGTRAP`/`__debugbreak`/`__builtin_trap`), compiled out under
  `INK_CONFIG_DIST`.
- **`ArenaAllocator` (`InkedArena`)**: mmap-backed bump/arena allocator
  with block chaining, alignment-aware allocation, and O(1) reset.
- **`ObjectPool<T, iSize>`**: contiguous-slab object pool with O(1)
  placement-new `acquire()` / destructor-calling `release()`, automatic
  slab expansion, and raw buffer access for zero-copy registration (e.g.
  io_uring fixed buffers).
- **`AlignedAllocator<T, Alignment>`**: std-compatible allocator producing
  over-aligned memory, usable directly with `std::vector` and other
  standard containers.
- **`RingBuffer`**: fixed-capacity byte ring buffer with contiguous
  zero-copy read/write buffer access, wrap-around handling, and move
  semantics.
- **`InkedList<T>`**: hand-rolled doubly linked list with push/pop from
  both ends, positional insert, index/value removal, and header-aware
  construction.
- **`InkixTree<T>`**: radix (compressed prefix) tree with insert/get/
  copy-get, returning `nullptr`/`std::optional` for missing keys.
- **`Queue<T>`**: thread-safe blocking queue with `push`/`push_bulk`,
  `wait_and_pop`, `try_pop`, `try_pop_for` (timed), and cooperative
  `shutdown()` to unblock waiters.
- **`TimerWheel`**: O(1) hashed timing wheel for session/connection
  timeouts, with `update`/`unlink`/`tick`/`processExpired`.
- **`ThreadPool`**: fixed-size worker pool with `std::future`-returning
  `submit()` for arbitrary callables and arguments.
- **`WorkerThread`**: single-thread task runner with start/stop/wake
  lifecycle, start/destruction callbacks, and a configurable stop policy
  (`WaitProcessFinish` joins; `WaitTimeout` detaches without blocking the
  caller).
- **`InkType`**: tagged-union dynamic value type covering all fixed-width
  integer/float types, `bool`, `char`, `ink_h` handles, and `std::string`,
  convertible to `std::variant`.
- **`EnhancedJson` / `EnhancedJsonUtils`**: `nlohmann::json`-derived
  wrapper with non-throwing `get`/`getPath` accessors with defaults,
  dot-path `setPath`, `filter`/`map`/`find`/`findAll`, chained
  `JsonQuery`/`select`, file and string (de)serialization, CBOR/MessagePack/
  BSON binary (de)serialization, merge/diff/patch, and type-inspection
  utilities.
- **`ArgParser`**: CLI argument parser with short/long flags, required vs.
  optional arguments with defaults, and auto-generated help text.
- **`InkOtp` (`ink::crypt::OTP`)**: XOR-cipher key generation/encryption/
  decryption utility with OS-entropy-seeded key generation and file
  read/write helpers. Not a cryptographically secure primitive; suitable
  for obfuscation and testing.
- **`LastWish`**: RAII scope-guard that runs a callback on construction
  and another on destruction.
- **`utils`**: shell command execution (`exec_command`), character/string
  to integer parsing (`cto_int`, `string_int`), and monotonic millisecond
  clock (`nowMillis`).
- **Build system**: CMake package with `ink::ink` target, presets for
  native Linux, Android (NDK), and WebAssembly (Emscripten) builds, and
  an install/export config for downstream `find_package(ink)` consumers.
- **Test suite**: assertion-based coverage of every module above, including
  concurrency stress coverage for the logger's lock-free file-writing path
  and regression coverage for previously fixed defects.

[0.2.0]: https://github.com/Arthu-RL/libink/releases/tag/0.2.0
[0.1.0]: https://github.com/Arthu-RL/libink/releases/tag/0.1.0
