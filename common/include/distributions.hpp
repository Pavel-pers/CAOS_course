#pragma once

#include <random>

struct UniformCharDistribution {
    using result_type = char;

    UniformCharDistribution(char lhs, char rhs)
        : impl_(static_cast<int>(lhs), static_cast<int>(rhs)) {
    }

    template <class Rng>
    char operator()(Rng& rng) {
        return static_cast<char>(impl_(rng));
    }

  private:
    std::uniform_int_distribution<int> impl_;
};
