#pragma once

#ifndef __linux__
#error "Only linux is supported for this problem"
#endif

#include <cstddef>
#include <utility>
#include <vector>

struct FileDescriptorsGuard {
    FileDescriptorsGuard();

    [[nodiscard]] bool TestDescriptorsState();

    size_t OpenFdsCount() const;

    ~FileDescriptorsGuard();

  private:
    bool ShouldWarnKCmp();
    bool TestSameFd(int fd1, int fd2);

    std::vector<std::pair<int, int>> copies;
    bool warned_kcmp_{false};
};
