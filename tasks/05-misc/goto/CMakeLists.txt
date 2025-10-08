add_catch_executable(test_goto count-substrings.cpp test.cpp)
target_link_libraries(test_goto PRIVATE caos_utils)

add_caos_executable(example_goto example.cpp)
