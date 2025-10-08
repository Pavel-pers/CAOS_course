#include "simple-allocator.hpp"

#include <syscalls.hpp>

#include <cstddef>
#include <sys/mman.h>
#include <type_traits>

class AllocatorState {
  public:
    void* Allocate16() {
        if (cur_free) {
            void* cur_ptr = cur_free;
            cur_free = next_free;
            return static_cast<void*>(cur_ptr);
        }

        if (cur_page && used + kBlockSize <= kPageSize) {
            char* ptr = cur_page + used;
            used += kBlockSize;
            return ptr;
        } else {
            return make_new_page();
        }
    }

    void Deallocate(void* ptr) {
        next_free = cur_free;
        cur_free = ptr;
    }

  private:
    void* make_new_page() {
        void* page = MMap(nullptr, kPageSize, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (page == MAP_FAILED) {
            return nullptr;
        }

        cur_page = static_cast<char*>(page);
        used = kBlockSize;
        return cur_page;
    }

    static constexpr size_t kBlockSize = 16;
    static constexpr size_t kPageSize = 1 << 12;

    void* cur_free;
    void* next_free;
    char* cur_page;
    size_t used;
};

static AllocatorState allocator_state_;
static_assert(std::is_trivially_constructible_v<AllocatorState>);

void* Allocate16() {
    return allocator_state_.Allocate16();
}

void Deallocate16(void* ptr) {
    allocator_state_.Deallocate(ptr);
}
