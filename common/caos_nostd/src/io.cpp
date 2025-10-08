#include <assert.hpp>
#include <io.hpp>
#include <strings.hpp>
#include <syscalls.hpp>

#include <unistd.h>

namespace nostd {

int WriteAll(int fd, const char* message, size_t len) {
    auto remaining = len;
    while (remaining > 0) {
        auto ret = Write(fd, message, remaining);
        if (ret == -1) {
            return ret;
        }

        auto written = static_cast<size_t>(ret);
        ASSERT_NO_REPORT(written <= remaining);
        remaining -= written;
        message += written;
    }

    return static_cast<int>(len);
}

void PrintTo(int fd, const char* message) {
    int written = WriteAll(fd, message, StrLen(message));
    ASSERT_NO_REPORT(written != -1);
}

void Print(const char* message) {
    PrintTo(STDOUT_FILENO, message);
}

void EPrint(const char* message) {
    PrintTo(STDERR_FILENO, message);
}

}  // namespace nostd

int WriteAll(int fd, const char* message, size_t len) {
    return nostd::WriteAll(fd, message, len);
}

void Print(const char* message) {
    nostd::Print(message);
}
