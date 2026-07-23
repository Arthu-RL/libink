# Threads
# - Native / Android NDK: resolves to -lpthread via CMake's FindThreads
# - Emscripten: resolves to the -pthread compile/link flag (no separate lib)
find_package(Threads REQUIRED)

find_package(nlohmann_json 3.12.0 REQUIRED)
