#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

struct Node {
    int32_t key;
    int32_t left_idx;
    int32_t right_idx;
};

constexpr size_t NodeSize = sizeof(Node);

Node read_node(int fd, const size_t idx) {
    off_t current_pos = lseek(fd, 0, SEEK_CUR);
    if (current_pos == -1) {
        std::cerr << "Error: unable to get Node: " << errno << std::endl;
        exit(1);
    }

    off_t target_pos = static_cast<off_t>(idx) * static_cast<off_t>(NodeSize);
    if (lseek(fd, target_pos, SEEK_SET) == -1) {
        std::cerr << "Error: unable to seek to position " << target_pos
                  << "errno: " << errno << std::endl;
        exit(1);
    }

    char buf[sizeof(Node)];
    size_t summary_bytes = 0;
    while (summary_bytes < NodeSize) {
        ssize_t bytes_read =
            read(fd, buf + summary_bytes, NodeSize - summary_bytes);
        if (bytes_read == -1) {
            std::cerr << "Unable to read Node at index: " << idx
                      << "  errno:" << errno << std::endl;
            exit(1);
        }
        if (bytes_read == 0) {
            std::cerr << "Unable to read Node at index: " << idx
                      << " file closed" << std::endl;
            exit(1);
        }
        summary_bytes += bytes_read;
    }

    if (lseek(fd, current_pos, SEEK_SET) == -1) {
        std::cerr << "Error: unable to restore file position" << std::endl;
        exit(1);
    }

    Node node;
    memcpy(&node, buf, sizeof(Node));
    return node;
}

void reverse_in_order_traversal(int fd, int32_t idx) {
    if (idx == 0) {
        return;
    }

    Node node = read_node(fd, idx);
    reverse_in_order_traversal(fd, node.right_idx);
    std::cout << node.key << " ";
    reverse_in_order_traversal(fd, node.left_idx);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "Error:   unable to open file" << std::endl;
        return 1;
    }

    Node root = read_node(fd, 0);
    reverse_in_order_traversal(fd, root.right_idx);
    std::cout << root.key << " ";
    reverse_in_order_traversal(fd, root.left_idx);
    std::cout << std::endl;
    close(fd);
}