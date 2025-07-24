#include "simple-allocator.hpp"

#include <assert.hpp>
#include <random.hpp>
#include <strings.hpp>
#include <syscalls.hpp>

#include <algorithm>
#include <array>

#define STRINGIFY_X(x) #x
#define STRINGIFY(x) STRINGIFY_X(x)

constexpr size_t kBlockSize = 16;

void Use(void* ptr, PCGRandom& rng) {
    auto values = reinterpret_cast<uint32_t*>(ptr);
    for (size_t i = 0; i < kBlockSize / sizeof(*values); ++i) {
        values[i] = rng();
    }
}

#define ASSERT_ALLOC(ptr, rng)                                                 \
    do {                                                                       \
        ASSERT(ptr != nullptr, "Allocation failed");                           \
        Use(ptr, rng);                                                         \
    } while (false)

void TestSimple(PCGRandom& rng) {
    void* obj1 = Allocate16();
    ASSERT_ALLOC(obj1, rng);
    void* obj2 = Allocate16();
    ASSERT_ALLOC(obj2, rng);

    Deallocate16(obj1);
    Deallocate16(obj2);
}

template <size_t N, size_t M>
void TestRepeatedRandomDeallocations(size_t iterations, PCGRandom& rng) {
    static_assert(M < N);

    std::array<void*, N> pool;
    for (size_t i = 0; i < M; ++i) {
        pool[i] = Allocate16();
        ASSERT_ALLOC(pool[i], rng);
    }

    for (size_t it = 0; it < iterations; ++it) {
        for (size_t j = M; j < N; ++j) {
            pool[j] = Allocate16();
            ASSERT_ALLOC(pool[j], rng);
        }

        std::shuffle(pool.begin(), pool.end(), rng);

        for (size_t j = M; j < N; ++j) {
            Deallocate16(pool[j]);
        }
    }

    for (size_t i = 0; i < M; ++i) {
        Deallocate16(pool[i]);
    }
}

#define RUN_TEST(test, ...)                                                    \
    do {                                                                       \
        Print("Running " #test "\n");                                          \
        test(__VA_ARGS__);                                                     \
    } while (false)

extern "C" [[noreturn]] void Main() {
    PCGRandom rng{321};

    RUN_TEST(TestSimple, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<32, 0>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<32, 16>), 1000, rng);

    RUN_TEST((TestRepeatedRandomDeallocations<2048, 0>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<2048, 1024>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<2048, 1536>), 1000, rng);
    RUN_TEST((TestRepeatedRandomDeallocations<2048, 1792>), 1000, rng);

    Exit(0);
}
