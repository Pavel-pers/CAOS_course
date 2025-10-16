#pragma once

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

struct Task {
    uint64_t idx;
    uint64_t L;
    uint64_t R;
};

struct Result {
    uint64_t idx;
    uint64_t value;
};

inline bool read_full(int fd, void* buf, size_t n) {
    auto* p = static_cast<unsigned char*>(buf);
    size_t left = n;
    while (left > 0) {
        ssize_t r = ::read(fd, p, left);
        if (r == 0) {
            return false;  // EOF
        }
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        p += r;
        left -= static_cast<size_t>(r);
    }
    return true;
}

inline bool write_full(int fd, const void* buf, size_t n) {
    auto* p = static_cast<const unsigned char*>(buf);
    size_t left = n;
    while (left > 0) {
        ssize_t r = ::write(fd, p, left);
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        p += r;
        left -= static_cast<size_t>(r);
    }
    return true;
}

template <class F>
inline uint64_t calc_range(uint64_t l, uint64_t r, const F& f) {
    uint64_t acc = l;
    for (uint64_t x = l + 1; x < r; ++x) {
        acc = f(acc, x);
    }
    return acc;
}

template <class F>
void consumer(int fd_input, int fd_output, F&& f) {
    Task t;
    while (read_full(fd_input, &t, sizeof(Task))) {
        if (t.L >= t.R) {
            continue;
        }
        uint64_t value = calc_range(t.L, t.R, f);
        Result r{t.idx, value};
        if (!write_full(fd_output, &r, sizeof(Result))) {
            break;
        }
    }
}

template <class F>
uint64_t JustCalculate(uint64_t from, uint64_t to, uint64_t init, F&& f) {
    uint64_t result = init;
    for (uint64_t i = from; i < to; i++) {
        result = f(result, i);
    }
    return result;
}

template <class F>
uint64_t Reduce(uint64_t from, uint64_t to, uint64_t init, F&& f,
                size_t max_parallelism) {
    if (from >= to) {
        return init;
    }

    F local_f(std::forward<F>(f));

    const uint64_t total = to - from;
    size_t count_of_procs =
        std::min<size_t>(max_parallelism, static_cast<size_t>(total));
    if (count_of_procs <= 1) {
        return JustCalculate(from, to, init, local_f);
    }
    if (total / count_of_procs < 200'000ull) {
        return JustCalculate(from, to, init, local_f);
    }

    int work_pipe_fd[2];
    int result_pipe_fd[2];
    if (pipe(work_pipe_fd) == -1) {
        std::cerr << "unluck :(" << std::endl;
        return JustCalculate(from, to, init, local_f);
    }
    if (pipe(result_pipe_fd) == -1) {
        std::cerr << "unluck :(" << std::endl;
        close(work_pipe_fd[0]);
        close(work_pipe_fd[1]);
        return JustCalculate(from, to, init, local_f);
    }

    size_t procs_created = 0;
    auto wait_children = [&]() {
        while (procs_created--) {
            wait(nullptr);
        }
    };

    for (; procs_created < max_parallelism; procs_created++) {
        pid_t child_pid = fork();
        if (child_pid < 0) {
            std::cerr << "threading unluck :(" << std::endl;
            break;
        }
        if (child_pid == 0) {
            close(work_pipe_fd[1]);
            close(result_pipe_fd[0]);
            consumer(work_pipe_fd[0], result_pipe_fd[1], local_f);
            close(work_pipe_fd[0]);
            close(result_pipe_fd[1]);
            _exit(0);
        }
    }

    close(work_pipe_fd[0]);
    close(result_pipe_fd[1]);

    if (procs_created == 0) {
        close(work_pipe_fd[1]);
        close(result_pipe_fd[0]);
        return JustCalculate(from, to, init, local_f);
    }

    const uint64_t tasks_per_proc = 16;
    const uint64_t target_tasks =
        std::max<uint64_t>(1, tasks_per_proc * procs_created);
    uint64_t chunk = (total + target_tasks - 1) / target_tasks;  // ceil
    if (chunk == 0) {
        chunk = 1;
    }

    uint64_t task_cnt = 0;
    for (uint64_t l = from; l < to; l += chunk) {
        uint64_t r = std::min<uint64_t>(l + chunk, to);
        Task t{task_cnt, l, r};
        if (!write_full(work_pipe_fd[1], &t, sizeof(Task))) {
            break;
        }
        ++task_cnt;
    }
    close(work_pipe_fd[1]);

    std::vector<uint64_t> parts(task_cnt, 0);
    uint64_t received = 0;
    while (received < task_cnt) {
        Result r;
        if (!read_full(result_pipe_fd[0], &r, sizeof(Result))) {
            close(result_pipe_fd[0]);
            wait_children();
            return JustCalculate(from, to, init, local_f);
        }
        if (r.idx < task_cnt) {
            parts[r.idx] = r.value;
            received++;
        }
    }
    close(result_pipe_fd[0]);
    wait_children();

    uint64_t acc = init;
    for (uint64_t v : parts) {
        acc = local_f(acc, v);
    }
    return acc;
}
