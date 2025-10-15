#pragma once

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <span>
#include <sys/mman.h>
#include <sys/stat.h>
#include <variant>

struct RingBuffer {
    using ElementType = int64_t;
    static constexpr size_t kPageSize = 1 << 12;

    static std::variant<RingBuffer, int> Create(size_t capacity) noexcept {
        if (capacity == 0) {
            capacity = 1;
        }

        size_t bytes_count = capacity * sizeof(ElementType);
        size_t B = (bytes_count + kPageSize - 1) & ~(kPageSize - 1);
        size_t item_cap = B / sizeof(ElementType);

        void* reserve =
            mmap(nullptr, 2 * B, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (reserve == MAP_FAILED) {
            std::cerr << "unluck :(";
            return errno;
        }

        void* origin_tmp = mmap(nullptr, B, PROT_READ | PROT_WRITE,
                                MAP_ANONYMOUS | MAP_SHARED, -1, 0);
        if (origin_tmp == MAP_FAILED) {
            std::cerr << "unluck  :(";
            munmap(reserve, 2 * B);
            return errno;
        }

        munmap(reserve, B);

        void* origin_buf =
            mremap(origin_tmp, B, B, MREMAP_MAYMOVE | MREMAP_FIXED, reserve);
        if (origin_buf == MAP_FAILED) {
            std::cerr << "unluck :(";
            munmap(origin_tmp, B);
            munmap(static_cast<char*>(reserve) + B, B);
            return errno;
        }

        munmap(static_cast<char*>(origin_buf) + B, B);

        void* mirror_buf = mremap(
            origin_buf, B, B, MREMAP_MAYMOVE | MREMAP_FIXED | MREMAP_DONTUNMAP,
            static_cast<char*>(origin_buf) + B);
        if (mirror_buf == MAP_FAILED) {
            std::cerr << "unluck :(";
            munmap(origin_buf, 2 * B);
            return errno;
        }

        return RingBuffer(static_cast<ElementType*>(origin_buf), B, item_cap);
    }

    [[nodiscard]] size_t Capacity() const {
        return item_cap_;
    }

    void PushBack(ElementType value) {
        size_t end = (head_ + size_) % item_cap_;
        origin_buf_[end] = value;
        size_++;
    }

    void PopBack() {
        size_--;
    }

    void PushFront(ElementType value) {
        size_t rend = (head_ + item_cap_ - 1) % item_cap_;
        origin_buf_[rend] = value;
        head_ = (head_ + item_cap_ - 1) % item_cap_;
        size_++;
    }

    void PopFront() {
        head_ = (head_ + 1) % item_cap_;
        size_--;
    }

    std::span<ElementType> Data() {
        return {origin_buf_ + head_, size_};
    }

    ~RingBuffer() {
        if (origin_buf_) {
            munmap(static_cast<void*>(origin_buf_), 2 * byte_cap_);
            origin_buf_ = nullptr;
        }
    }

    RingBuffer(const RingBuffer&) = delete;

    RingBuffer& operator=(const RingBuffer&) = delete;

    RingBuffer(RingBuffer&& other) noexcept
        : origin_buf_(other.origin_buf_), byte_cap_(other.byte_cap_),
          item_cap_(other.item_cap_), head_(other.head_), size_(other.size_) {
        other.origin_buf_ = nullptr;
        other.item_cap_ = 0;
        other.byte_cap_ = 0;
        other.head_ = 0;
        other.size_ = 0;
    }

    RingBuffer& operator=(RingBuffer&& other) noexcept {
        if (this != &other) {
            if (origin_buf_) {
                ::munmap(static_cast<void*>(origin_buf_), 2 * byte_cap_);
            }
            origin_buf_ = other.origin_buf_;
            byte_cap_ = other.byte_cap_;
            item_cap_ = other.item_cap_;
            size_ = other.size_;
            head_ = other.head_;

            other.origin_buf_ = nullptr;
            other.byte_cap_ = 0;
            other.item_cap_ = 0;
            other.size_ = 0;
            other.head_ = 0;
        }
        return *this;
    }

  private:
    ElementType* origin_buf_;
    size_t byte_cap_;
    size_t item_cap_;
    size_t head_;
    size_t size_;

    RingBuffer(ElementType* origin_buf_ptr, size_t byte_capacity,
               size_t item_capacity)
        : origin_buf_(origin_buf_ptr), byte_cap_(byte_capacity),
          item_cap_(item_capacity), head_(0), size_(0) {
        head_ = 0;
        size_ = 0;
    }
};