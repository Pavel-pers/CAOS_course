add_executable(test_simple_alloc simple-allocator.cpp test.cpp)
target_link_libraries(test_simple_alloc PUBLIC caos_nostd)
