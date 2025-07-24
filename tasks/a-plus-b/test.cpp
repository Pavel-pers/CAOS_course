#include "a-plus-b.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Simple") {
    CHECK(Sum(0, 0) == 0);
    CHECK(Sum(10, 123) == 133);

    CHECK(Sum(1UL << 31, 1UL << 31) == 0);
}
