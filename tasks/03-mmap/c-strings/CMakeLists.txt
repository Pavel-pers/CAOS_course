add_caos_library(c_strings_nostd STATIC no-alloc.cpp)
target_include_directories(c_strings_nostd PUBLIC .)
target_link_libraries(c_strings_nostd PRIVATE caos_nostd_flags)

add_catch_executable(test_c_strings alloc.cpp test.cpp)
target_link_libraries(test_c_strings PRIVATE benchmark c_strings_nostd)

add_caos_executable(test_c_strings_nostd test-nostd.cpp)
target_link_libraries(test_c_strings_nostd PRIVATE caos_nostd c_strings_nostd)
