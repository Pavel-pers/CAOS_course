#include "simple-allocator.hpp"

#include <syscalls.hpp>

#include <cstddef>
#include <sys/mman.h>
#include <type_traits>

class AllocatorState {
  public:
    void* Allocate16() {
        if (free_list) {
            FreeListNode* n = free_list;
            free_list = n->next;
            return static_cast<void*>(n);
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
        auto* n = static_cast<FreeListNode*>(ptr);
        n->next = free_list;
        free_list = n;
    }

  private:
    struct FreeListNode {
        FreeListNode* next;
    };

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

    FreeListNode* free_list;
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
