#pragma once

#include <compare>  // IWYU pragma: export
#include <cstddef>

struct SubstringsCount {
    size_t rop_num = 0;
    size_t os_num = 0;

    auto operator<=>(const SubstringsCount& other) const = default;
};

SubstringsCount CountSubstrings(const char* s);
