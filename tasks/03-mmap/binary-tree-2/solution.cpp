#include <cstdint>
#include <iostream>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

struct Node {
    int32_t key;
    int32_t left_idx;
    int32_t right_idx;
};

void dfs(int32_t idx, const Node* buf) {
    Node cur_node = buf[idx];
    if (cur_node.right_idx != 0) {
        dfs(cur_node.right_idx, buf);
    }
    std::cout << cur_node.key << ' ';
    if (cur_node.left_idx != 0) {
        dfs(cur_node.left_idx, buf);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <file>" << std::endl;
        return 1;
    }

    const ssize_t fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "Failed to open file " << argv[1] << std::endl;
        return 1;
    }

    struct stat st {};

    if (fstat(fd, &st) == -1) {
        std::cerr << "Failed to stat file " << argv[1] << std::endl;
        return 1;
    }

    void* buf = mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE,
                     static_cast<int>(fd), 0);
    if (buf == MAP_FAILED) {
        std::cerr << "Failed to map file " << argv[1] << std::endl;
        return 1;
    }

    const auto data = static_cast<const Node*>(buf);
    dfs(0, data);

    if (munmap(buf, st.st_size) == -1) {
        std::cerr << "Failed to unmap file " << argv[1] << std::endl;
        return 1;
    }

    if (close(fd) == -1) {
        std::cerr << "Failed to close file " << argv[1] << std::endl;
        return 1;
    }
}
