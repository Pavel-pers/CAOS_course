#pragma once

#include <io.hpp>
#include <syscalls.hpp>

#define STRINGIFY_X(x) #x
#define STRINGIFY(x) STRINGIFY_X(x)

// Variation of assert that makes debugging much more fun
#define ASSERT_NO_REPORT(cond)                                                 \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::Exit(2);                                                         \
        }                                                                      \
    } while (false)

#define ASSERT(cond, message)                                                  \
    do {                                                                       \
        if (!(cond)) {                                                         \
            ::Print("Line " STRINGIFY(__LINE__) " Condition " #cond            \
                                                " failed: " message "\n");     \
            ::Exit(1);                                                         \
        }                                                                      \
    } while (0)
