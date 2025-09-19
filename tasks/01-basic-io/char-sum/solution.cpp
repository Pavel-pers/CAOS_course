#include <cctype>  // std::isdigit
#include <cstdio>
#include <cstdlib>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

ssize_t sum_bytes(const int fd) {
    static constexpr int buck_size = 8;

    size_t sum = 0;
    while (true) {
        char buf[buck_size];
        ssize_t pkg_size = read(fd, buf, buck_size);
        if (pkg_size == -1) {
            return -1;
        } else if (pkg_size == 0) {
            return static_cast<ssize_t>(sum);
        }

        for (char* i = buf; i < buf + pkg_size; i++) {
            if (isdigit(*i)) {
                sum += *i - '0';
            }
        }
    }
}

bool write_int(const int fd, uint64_t num) {
    char num_str[19];
    size_t digit_count = 0;
    for (int i = 0; num > 0; i++) {
        num_str[i] = static_cast<char>((num % 10) + '0');
        num /= 10;
        digit_count++;
    }

    for (size_t i = 0; i < digit_count; i++) {
        ssize_t pkg_size = write(fd, &num_str[digit_count - i - 1], 1);
        if (pkg_size == -1 || pkg_size == 0) {
            return false;
        }
    }
    return true;
}

int main() {
    ssize_t sum = sum_bytes(0);
    if (sum == -1) {
        throw std::runtime_error("file read error: " + std::to_string(errno));
    }
    if (!write_int(1, sum)) {
        throw std::runtime_error("write error: " + std::to_string(errno));
    }
}
