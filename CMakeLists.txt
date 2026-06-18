cmake_minimum_required(VERSION 4.3.3)
project(ink LANGUAGES CXX VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE INK_SOURCES "src/*.cpp")
file(GLOB_RECURSE INK_HEADERS "include/ink/*.h" "include/ink/*.hpp")

# Create the library
add_library(${PROJECT_NAME} STATIC ${INK_SOURCES} ${INK_HEADERS})

target_include_directories(${PROJECT_NAME} PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)

# Dependencies
find_package(Threads REQUIRED)
target_link_libraries(${PROJECT_NAME} PUBLIC Threads::Threads)

if(TARGET nlohmann_json::nlohmann_json)
  target_link_libraries(${PROJECT_NAME} PUBLIC nlohmann_json::nlohmann_json)
endif()

# Build Tests
if(NOT DEFINED INK_BUILD_TESTS)
  set(INK_BUILD_TESTS ON)
endif()

if(INK_BUILD_TESTS)
  enable_testing()
  add_executable(${PROJECT_NAME}_tests test/test_main.cpp)
  target_link_libraries(${PROJECT_NAME}_tests PRIVATE ${PROJECT_NAME})
  add_test(NAME ${PROJECT_NAME}_tests COMMAND ${PROJECT_NAME}_tests)
endif()

# Compiler Flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_options(${PROJECT_NAME} PRIVATE -g -O0 -Wall -Wextra -Wpedantic)
else()
  target_compile_options(${PROJECT_NAME} PRIVATE -O3 -DNDEBUG)
endif()