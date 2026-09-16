add_executable(ink_tests test_main.cpp)
target_link_libraries(ink_tests PRIVATE ink::ink)
add_test(NAME ink_tests COMMAND ink_tests)

add_executable(ink_runtime_storage_tests test_runtime_storage.cpp)
target_link_libraries(ink_runtime_storage_tests PRIVATE ink::ink)
add_test(NAME ink_runtime_storage_tests COMMAND ink_runtime_storage_tests)
