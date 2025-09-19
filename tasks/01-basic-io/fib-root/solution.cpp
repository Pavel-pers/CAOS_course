#include <cstdint>  // int64_t
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

static constexpr size_t max_fib_root = 100;

bool isdigit16(char c) {
    return ('0' <= c && c <= '9') || ('a' <= c && c <= 'f');
}

size_t parse_digit(char c) {
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('a' <= c && c <= 'f') {
        return 10 + c - 'a';
    }
    return 0;
}

char make_digit(uint8_t num) {
    if (num <= 9) {
        return static_cast<char>('0' + num);
    } else {
        return static_cast<char>('a' + num - 10);
    }
}

ssize_t read_int(int fd, uint64_t& num) {
    num = 0;
    bool got_digit = false;

    while (true) {
        char buf[1];
        ssize_t pkg_size = read(fd, buf, 1);

        if (pkg_size == -1) {
            return -1;
        }
        if (pkg_size == 0) {
            return 0;
        }

        char inp = buf[0];
        if (!isdigit16(inp)) {
            if (got_digit) {
                break;
            } else {
                continue;
            }
        } else {
            got_digit = true;
            num *= 16;
            num += parse_digit(inp);
        }
    }
    return 1;
}

bool write_int(const int fd, uint64_t num) {
    char num_str[19];
    size_t digit_count = 0;
    for (int i = 0; num > 0; i++) {
        num_str[i] = make_digit(num % 16);
        num /= 16;
        digit_count++;
    }
    if (digit_count == 0) {
        num_str[0] = '0';
        digit_count++;
    }

    for (size_t i = 0; i < digit_count; i++) {
        ssize_t pkg_size = write(fd, &num_str[digit_count - i - 1], 1);
        if (pkg_size == -1 || pkg_size == 0) {
            return false;
        }
    }
    char newline = '\n';
    ssize_t pkg_size = write(fd, &newline, 1);
    if (pkg_size == -1 || pkg_size == 0) {
        return false;
    } else {
        return true;
    }
}

uint64_t* compute_fib_numbers(uint64_t fib_numbers[]) {
    fib_numbers[0] = 0;
    fib_numbers[1] = 1;
    size_t size = 2;

    for (size_t i = 2; i <= max_fib_root; i++) {
        if (std::numeric_limits<uint64_t>::max() - fib_numbers[i - 1] <
            fib_numbers[i - 2]) {
            size = i;
            break;
        }
        fib_numbers[i] = fib_numbers[i - 1] + fib_numbers[i - 2];
    }

    return fib_numbers + size;
}

size_t get_fib_root(uint64_t x) {
    static uint64_t fib_numbers[max_fib_root + 1];
    static size_t fib_numbers_size = 0;

    if (fib_numbers_size == 0) {
        fib_numbers_size = compute_fib_numbers(fib_numbers) - fib_numbers;
    }

    for (size_t i = 0; i < fib_numbers_size; i++) {
        if (fib_numbers[i] > x) {
            return i - 2;
        }
    }
    return fib_numbers_size - 2;
}

int main() {
    ssize_t read_flag = 1;
    uint64_t num = 0;
    read_flag = read_int(0, num);

    while (read_flag == 1) {
        size_t root = get_fib_root(num);
        if (!write_int(1, root)) {
            throw std::runtime_error("write error");
        }

        read_flag = read_int(0, num);
    }

    if (read_flag == -1) {
        throw std::runtime_error("read error");
    }
}
