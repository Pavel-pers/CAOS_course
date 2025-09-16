#pragma once

#include "internal_assert.hpp"

#include <sys/resource.h>
#include <utility>

struct RLimGuard {
    RLimGuard(int limit_type, rlim_t new_limit) : limit_type_(limit_type) {
        prev_limit_ = ModifySoftLimit(limit_type_, new_limit);
    }

    RLimGuard(const RLimGuard&) = delete;
    RLimGuard(RLimGuard&&) = delete;

    RLimGuard& operator=(const RLimGuard&) = delete;
    RLimGuard& operator=(RLimGuard&&) = delete;

    void Reset() {
        auto limit_type = std::exchange(limit_type_, -1);
        if (limit_type == -1) {
            return;
        }
        ModifySoftLimit(limit_type, prev_limit_);
    }

    ~RLimGuard() {
        Reset();
    }

  private:
    static rlim_t ModifySoftLimit(int limit_type, rlim_t new_limit) {
        struct rlimit limit{};
        {
            int ret = getrlimit(limit_type, &limit);
            INTERNAL_ASSERT(ret != -1);
        }

        INTERNAL_ASSERT(new_limit <= limit.rlim_max);
        auto prev_limit = limit.rlim_cur;
        limit.rlim_cur = new_limit;

        {
            int ret = setrlimit(limit_type, &limit);
            INTERNAL_ASSERT(ret != -1);
        }
        return prev_limit;
    }

    int limit_type_;
    rlim_t prev_limit_;
};
