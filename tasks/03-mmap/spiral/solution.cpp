#include <cerrno>
#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <out.bin> <rows> <cols>\n";
        return 1;
    }

    const char* path = argv[1];
    const uint32_t rows = std::stoi(argv[2]);
    const uint32_t cols = std::stoi(argv[3]);

    size_t cells = static_cast<size_t>(rows) * static_cast<size_t>(cols);

    size_t bytes = cells * static_cast<__uint128_t>(sizeof(int32_t));

    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        std::cerr << "open: " << std::strerror(errno) << "\n";
        return 1;
    }

    if (ftruncate(fd, static_cast<off_t>(bytes)) == -1) {
        std::cerr << "ftruncate: " << std::strerror(errno) << "\n";
        close(fd);
        return 1;
    }

    void* map = mmap(nullptr, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        std::cerr << "mmap: " << std::strerror(errno) << "\n";
        close(fd);
        return 1;
    }

    int32_t* it = static_cast<int32_t*>(map);
    auto idx = [cols](size_t r, size_t c) -> size_t { return r * cols + c; };

    size_t top = 0, left = 0;
    size_t bottom = rows - 1;
    size_t right = cols - 1;

    int32_t val = 1;
    while (true) {
        for (size_t c = left; c <= right; ++c) {
            it[idx(top, c)] = val++;
        }
        if (++top > bottom) {
            break;
        }

        for (size_t r = top; r <= bottom; ++r) {
            it[idx(r, right)] = val++;
        }
        if (right-- == 0 || left > right) {
            break;
        }
        for (size_t c = right + 1; c-- > left;) {
            it[idx(bottom, c)] = val++;
        }
        if (bottom-- == 0 || top > bottom) {
            break;
        }

        for (size_t r = bottom + 1; r-- > top;) {
            it[idx(r, left)] = val++;
        }
        if (++left > right) {
            break;
        }
    }

    if (msync(map, bytes, MS_SYNC) == -1) {
        std::cerr << "msync: " << std::strerror(errno) << "\n";
    }

    if (munmap(map, bytes) == -1) {
        std::cerr << "munmap: " << std::strerror(errno) << "\n";
    }
    if (close(fd) == -1) {
        std::cerr << "close: " << std::strerror(errno) << "\n";
    }
    return 0;
}
