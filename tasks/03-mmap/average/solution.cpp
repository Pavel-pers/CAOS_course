#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

using DataType = double;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::runtime_error("missed file name");
    }

    ssize_t fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        throw std::runtime_error("failed to open file");
    }

    struct stat st {};

    if (fstat(fd, &st) == -1) {
        throw std::runtime_error("failed to stat file");
    }

    size_t stat_size = st.st_size;
    if (stat_size % sizeof(DataType) != 0) {
        throw std::runtime_error(
            "file size is not a multiple of sizeof(DataType));");
    }

    size_t count = stat_size / sizeof(DataType);
    void* buf = mmap(nullptr, stat_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (buf == MAP_FAILED) {
        throw std::runtime_error("error mapping file");
    }

    auto data = static_cast<const DataType*>(buf);
    DataType sum = 0;
    for (size_t i = 0; i < count; i++) {
        sum += data[i];
    }

    DataType avg = sum / count;
    std::cout << std::hexfloat << avg << std::endl;

    if (munmap(buf, stat_size) == -1) {
        throw std::runtime_error("failed to unmap file");
    }
    if (close(fd) == -1) {
        throw std::runtime_error("failed to close file");
    }
}
