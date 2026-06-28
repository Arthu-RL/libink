add_executable(ink_tests test_main.cpp)

target_link_libraries(ink_tests PRIVATE ink::ink)

add_test(NAME ink_tests COMMAND ink_tests)