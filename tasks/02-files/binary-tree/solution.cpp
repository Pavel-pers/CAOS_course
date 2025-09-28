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
    char buf[sizeof(Node)];
    size_t summary_bytes = 0;
    while (summary_bytes < NodeSize) {
        ssize_t bytes_read =
            pread(fd, buf, NodeSize - summary_bytes,
                  static_cast<off_t>(idx * NodeSize + summary_bytes));
        if (bytes_read == -1) {
            std::cerr << "Unable to read Node at index: " << idx
                      << " erno:" << errno << std::endl;
            exit(1);
        }
        if (bytes_read == 0) {
            std::cerr << "Unable to read Node at index: " << idx
                      << " file clodsed" << std::endl;
            exit(1);
        }
        summary_bytes += bytes_read;
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
        std::cerr << "Error: unable to open file" << std::endl;
        return 1;
    }

    Node root = read_node(fd, 0);
    reverse_in_order_traversal(fd, root.right_idx);
    std::cout << root.key << " ";
    reverse_in_order_traversal(fd, root.left_idx);
    std::cout << std::endl;
    close(fd);
}