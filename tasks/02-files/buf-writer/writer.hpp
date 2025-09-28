#pragma once

#include <cerrno>
#include <cstring>
#include <string_view>
#include <unistd.h>

struct BufWriter {
    BufWriter(const int fd, const size_t buf_capacity)
        : buf_capacity_(buf_capacity), fd_(fd) {
        data_ = new char[buf_capacity];
        buf_size_ = 0;
    }

    // Non-copyable
    BufWriter(const BufWriter&) = delete;

    BufWriter& operator=(const BufWriter&) = delete;

    // Non-movable
    BufWriter(BufWriter&&) = delete;

    BufWriter& operator=(BufWriter&&) = delete;

    int Write(std::string_view data) {
        while (!data.empty()) {
            size_t push_prefix_size =
                std::min(data.size(), buf_capacity_ - buf_size_);
            std::string_view push_prefix = data.substr(0, push_prefix_size);
            data.remove_prefix(push_prefix_size);

            memcpy(data_ + buf_size_, push_prefix.data(), push_prefix_size);
            buf_size_ += push_prefix_size;

            if (buf_size_ == buf_capacity_) {
                if (const int flush_code = Flush()) {
                    return flush_code;
                }
            }
        }
        return 0;
    }

    int Flush() {
        size_t summary_written = 0;
        while (summary_written < buf_size_) {
            const ssize_t written = write(fd_, data_ + summary_written,
                                          buf_size_ - summary_written);
            if (written < 0) {
                return -errno;
            }
            summary_written += written;
        }
        buf_size_ = 0;
        return 0;
    }

    ~BufWriter() {
        Flush();
        delete[] data_;
    }

  private:
    const size_t buf_capacity_;
    int fd_;
    char* data_;
    size_t buf_size_;
};