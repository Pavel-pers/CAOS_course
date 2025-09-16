#include <fcntl.h>
#include <iostream>
#include <unistd.h>

#include <unused.hpp>  // TODO: remove before flight.

struct Node {
    int32_t key;
    int32_t left_idx;
    int32_t right_idx;
};

int main(int argc, char* argv[]) {
    UNUSED(argc, argv);  // TODO: remove before flight.
}
