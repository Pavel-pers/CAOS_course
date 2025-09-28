add_catch_executable(test_c_strings alloc.cpp no-alloc.cpp test.cpp)
target_link_libraries(test_c_strings PRIVATE benchmark)

add_caos_executable(test_c_strings_nostd no-alloc.cpp test-nostd.cpp)
target_link_libraries(test_c_strings_nostd PRIVATE caos_nostd)
