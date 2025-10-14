#pragma once

#include <macros.hpp>
#include <utility>

template <class F>
struct Defer {
    Defer(F&& f) : f_(std::forward<F>(f)) {
    }

    ~Defer() {
        f_();
    }

  private:
    F f_;
};

#define DEFER Defer UNIQUE_ID(_defer_guard) = [&]()
