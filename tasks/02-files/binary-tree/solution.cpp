#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

struct Node {
    int32_t key;
    int32_t left_idx;
    int32_t right_idx;
};

constexpr size_t NodeSize = sizeof(Node);

static bool pread_full(int fd, void* buf, size_t n, off_t off) {
    size_t done = 0;
    while (done < n) {
        ssize_t r =
            pread(fd, static_cast<char*>(buf) + done, n - done, off + done);
        if (r == 0) {
            return false;
        }
        if (r < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        done += static_cast<size_t>(r);
    }
    return true;
}

static bool read_node(int fd, int32_t idx, size_t node_count, Node& out) {
    if (idx < 0 || static_cast<size_t>(idx) >= node_count) {
        std::cerr << "node index out of range: " << idx << '\n';
        return false;
    }
    const off_t off = static_cast<off_t>(idx) * static_cast<off_t>(NodeSize);
    char buf[NodeSize];
    if (!pread_full(fd, buf, NodeSize, off)) {
        std::cerr << "pread failed at index " << idx;
        return false;
    }
    std::memcpy(&out, buf, NodeSize);
    return true;
}

static void reverse_in_order_traversal(int fd, int32_t idx, size_t node_count) {
    if (idx == 0) {
        return;
    }

    std::vector<Node> st;
    st.reserve(1024);

    int32_t cur = idx;
    Node n;

    while (cur != 0 || !st.empty()) {
        while (cur != 0) {
            if (!read_node(fd, cur, node_count, n)) {
                std::cerr << "Error: fail to read file" << idx
                          << " (errno=" << errno << ")\n";
            }
            st.push_back(n);
            cur = n.right_idx;
        }

        n = st.back();
        st.pop_back();

        std::cout << n.key << ' ';
        cur = n.left_idx;
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        return 1;
    }

    int fd = open(argv[1], O_RDONLY);
    if (fd == -1) {
        std::cerr << "unable to open file (errno=" << errno << ")\n";
        return 1;
    }

    struct stat st{};

    if (fstat(fd, &st) == -1) {
        std::cerr << "fstat failed (errno=" << errno << ")\n";
        close(fd);
        return 1;
    }
    off_t filesize = st.st_size;

    if (filesize < static_cast<off_t>(NodeSize) ||
        (filesize % static_cast<off_t>(NodeSize)) != 0) {
        std::cerr << "Eror with file size\n";
        close(fd);
        return 1;
    }

    const size_t node_count =
        static_cast<size_t>(filesize / static_cast<off_t>(NodeSize));

    Node root;
    if (!read_node(fd, 0, node_count, root)) {
        close(fd);
        return 1;
    }

    reverse_in_order_traversal(fd, root.right_idx, node_count);
    std::cout << root.key << ' ';
    reverse_in_order_traversal(fd, root.left_idx, node_count);
    std::cout << '\n';

    close(fd);
    return 0;
}
