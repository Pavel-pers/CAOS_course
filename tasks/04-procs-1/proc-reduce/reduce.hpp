#pragma once

#include <cstdint>
#include <numeric>
#include <ranges>
#include <unistd.h>
#include <iostream>
#include <sys/wait.h>
#include <vector>


template <class F>
void consumer(int fd_input, int fd_output, F&& f) {
    dup2(fd_input, STDIN_FILENO);
    dup2(fd_output, STDOUT_FILENO);
    int index, lhs, rhs;
    while (std::cin >> index >> lhs >> rhs) {
        std::cout << index << ' ' << f(lhs, rhs) << '\n';
    }
}

template <class F>
uint64_t JustCalculate(uint64_t from, uint64_t to, uint64_t init, F&& f) {
    uint64_t result = init;
    for (size_t i = from; i < to; i++) {
        result = f(result, i);
    }
    return result;
}

template <class F>
uint64_t Reduce(uint64_t from, uint64_t to, uint64_t init, F&& f,
                size_t max_parallelism) {
    max_parallelism = std::min(max_parallelism, to - from);

    int work_pipe_fd[2];
    int result_pipe_fd[2];
    if (pipe(work_pipe_fd) == -1) {
        std::cerr << "unluck :(" << std::endl;
        return JustCalculate(from, to, init, f);
    }
    if (pipe(result_pipe_fd) == -1) {
        std::cerr << "unluck :(" << std::endl;
        close(work_pipe_fd[0]);
        close(work_pipe_fd[1]);
        return JustCalculate(from, to, init, f);
    }

    size_t count_of_childs = 0;

    auto close_pipes = [&]() {
        close(work_pipe_fd[0]);
        close(work_pipe_fd[1]);
        close(result_pipe_fd[0]);
        close(result_pipe_fd[1]);
    };

    auto wait_children = [&]() {
        while (count_of_childs--) {
            wait(nullptr);
        }
    };

    for (size_t i = 0; i < max_parallelism; i++) {
        pid_t child_pid = fork();
        if (child_pid == -1) {
            std::cerr << "threading unluck :(" << std::endl;
            close_pipes();
            wait_children();
            return JustCalculate(from, to, init, f);
        }
        if (child_pid == 0) {
            consumer(work_pipe_fd[0], result_pipe_fd[1], f);
            exit(0);
        } else {
            count_of_childs++;
        }
    }

    dup2(work_pipe_fd[1], STDOUT_FILENO);
    dup2(result_pipe_fd[0], STDIN_FILENO);

    size_t res_size = to - from + 1;
    std::vector<uint64_t>res(to - from + 1);

    res[0] = init;
    for (size_t i = 1; i < res.size(); i++) {
        res[i] = from + i - 1;
    }
    std::iota(res.begin(), res.end(), from);
    while (res_size > 1) {
        for (size_t i = 0; i * 2 < res_size; i++) {
            std::cout << i << ' ' << res[i * 2] << ' ' << res[i * 2 + 1] << '\n';
        }
        res_size = (res_size + 1) / 2;

        for (size_t i = 0; i * 2 < res_size; i++) {
            std::cin >> res[i];
        }
    }

    close_pipes();
    wait_children();
    return res[0];
}