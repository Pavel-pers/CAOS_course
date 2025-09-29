#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

struct Node {
    int32_t key;
    int32_t left_idx;
    int32_t right_idx;
};

constexpr size_t NodeSize = sizeof(Node);
static_assert(sizeof(Node) == 12, "Unexpected Node size");

Node read_node(int fd, const size_t idx, const size_t node_count) {
    if (static_cast<size_t>(idx) >= node_count) {
        abort();
    }

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

void reverse_in_order_traversal(int fd, int32_t idx, size_t node_count) {
    if (idx == 0) {
        return;
    }

    std::vector<Node> st;
    st.reserve(1024);

    int32_t cur = idx;
    while (cur != 0 || !st.empty()) {
        while (cur != 0) {
            Node n = read_node(fd, cur, node_count);
            st.push_back(n);
            cur = n.right_idx;
        }

        Node n = st.back();
        st.pop_back();

        std::cout << n.key << " ";
        cur = n.left_idx;
    }
}

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>\n";
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "Error: unable to open file\n";
        return 1;
    }

    off_t filesize = lseek(fd, 0, SEEK_END);
    if (filesize == -1) {
        std::cerr << "Error: unable to get file size\n";
        close(fd);
        return 1;
    }
    if (filesize % static_cast<off_t>(NodeSize) != 0) {
        std::cerr << "Error: corrupted file (size not multiple of Node)\n";
        close(fd);
        return 1;
    }
    size_t node_count =
        static_cast<size_t>(filesize / static_cast<off_t>(NodeSize));
    if (node_count == 0) {
        std::cout << "\n";
        close(fd);
        return 0;
    }

    Node root = read_node(fd, 0, node_count);
    reverse_in_order_traversal(fd, root.right_idx, node_count);
    std::cout << root.key << " ";
    reverse_in_order_traversal(fd, root.left_idx, node_count);
    std::cout << '\n';

    close(fd);
    return 0;
}