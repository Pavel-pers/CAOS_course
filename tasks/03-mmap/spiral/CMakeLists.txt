add_caos_executable(solution_spiral solution.cpp wrappers.cpp)
target_link_options(
    solution_spiral
    PRIVATE
    -Wl,-wrap=main
)
