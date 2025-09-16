#pragma once

#ifndef __linux__
#error "Only linux is supported for this problem"
#endif

#include "internal_assert.hpp"

#include <algorithm>
#include <filesystem>
#include <unistd.h>
#include <vector>

#include <fcntl.h>
#include <linux/kcmp.h>
#include <sys/resource.h>
#include <sys/syscall.h>

namespace fs = std::filesystem;

namespace detail {

inline std::vector<int> GetOpenFileDescriptors() {
    std::vector<int> fds_candidates;
    for (auto dent : fs::directory_iterator{"/proc/self/fd"}) {
        fds_candidates.push_back(std::stoi(dent.path().filename()));
    }
    std::vector<int> fds;
    for (auto fd : fds_candidates) {
        int ret = fcntl(fd, F_GETFD);
        if (ret == -1) {
            continue;
        }
        fds.push_back(fd);
    }
    std::sort(fds.begin(), fds.end());

    return fds;
}

inline bool TestSameFd(int fd1, int fd2) {
    int pid = getpid();
    int ret = syscall(SYS_kcmp, pid, pid, KCMP_FILE, fd1, fd2);
    if (ret == -1) {
        int err = errno;
        INTERNAL_ASSERT(err != EBADF);
        return false;
    }
    return ret == 0;
}

inline size_t MaxFdNum() {
    struct rlimit limits{};

    int r = ::getrlimit(RLIMIT_NOFILE, &limits);
    INTERNAL_ASSERT(r != -1);
    return limits.rlim_cur;
}

}  // namespace detail

struct FileDescriptorsGuard {
    FileDescriptorsGuard() {
        auto fds = detail::GetOpenFileDescriptors();
        if (fds.empty()) {
            return;
        }

        auto max_fd_num = detail::MaxFdNum();
        INTERNAL_ASSERT(fds.back() + fds.size() < max_fd_num);

        auto cur_fd_num = static_cast<int>(max_fd_num - 1);
        for (auto fd : fds) {
            int ret = dup2(fd, cur_fd_num);
            INTERNAL_ASSERT(ret != -1);
            copies.emplace_back(fd, cur_fd_num);
            --cur_fd_num;
        }
    }

    [[nodiscard]] bool TestDescriptorsState() const {
        for (auto [from, to] : copies) {
            if (!detail::TestSameFd(from, to)) {
                WARN("Descriptors " << from << " and " << to
                                    << " don't refer to the same file");
                return false;
            }
        }

        auto fds = detail::GetOpenFileDescriptors();
        auto expected = copies.size() * 2;
        if (fds.size() != expected) {
            WARN("Descriptors count mismatch. Expected "
                 << expected << ", actually " << fds.size());
            return false;
        }
        return true;
    }

    ~FileDescriptorsGuard() {
        for (auto [_, fd] : copies) {
            close(fd);
        }
    }

  private:
    std::vector<std::pair<int, int>> copies;
};
