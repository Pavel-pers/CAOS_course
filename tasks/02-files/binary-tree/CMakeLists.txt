add_caos_executable(solution_binary_tree solution.cpp wrappers.cpp)
target_link_options(
    solution_binary_tree
    PRIVATE
    -Wl,-wrap=read
    -Wl,-wrap=pread
    -Wl,-wrap=__read_chk
    -Wl,-wrap=__pread_chk
)
