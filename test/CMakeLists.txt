add_executable(ink_tests test_main.cpp)
target_link_libraries(ink_tests PRIVATE ink::ink)
add_test(NAME ink_tests COMMAND ink_tests)

add_executable(ink_runtime_storage_tests test_runtime_storage.cpp)
target_link_libraries(ink_runtime_storage_tests PRIVATE ink::ink)
add_test(NAME ink_runtime_storage_tests COMMAND ink_runtime_storage_tests)

if(NOT MSVC)
    add_library(ink_base_include_free OBJECT test_base_include_free.cpp)
    target_include_directories(ink_base_include_free PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(ink_base_include_free PRIVATE -nostdinc -nostdinc++)
endif()
