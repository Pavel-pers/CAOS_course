#pragma once

#include <utility>

namespace detail {

void DoNotOptimize(const void*);

}

void DoNotReorder();

template <class T>
T&& DoNotOptimize(T&& v) {
    detail::DoNotOptimize(&v);
    return std::forward<T>(v);
}
