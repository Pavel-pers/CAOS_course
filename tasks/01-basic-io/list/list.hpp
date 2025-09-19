#pragma once

#include <cstdint>  // size_t
#include <cstdio>
#include <cstdlib>
#include <utility>

struct ListNode {
    int value;
    ListNode* prev;
    ListNode* next;
};

class List {
  public:
    // Non-copyable
    List(const List&) = delete;
    List& operator=(const List&) = delete;

    List(List&& other) : List() {
        Swap(other);
    }

    List& operator=(List&& other) {
        List tmp{std::move(other)};
        Swap(tmp);
        return *this;
    }

    List() {
        front = nullptr;
        back = nullptr;
        size = 0;
    }

    ~List() {
        Clear();
    }

    void PushBack(int value) {
        if (size == 0) {
            auto new_node = new ListNode(value, nullptr, nullptr);
            front = new_node;
            back = new_node;
        } else {
            auto* new_node = new ListNode(value, back, nullptr);
            back->next = new_node;
            back = new_node;
        }
        size++;
    }

    void PushFront(int value) {
        if (size == 0) {
            auto new_node = new ListNode(value, nullptr, nullptr);
            front = new_node;
            back = new_node;
        } else {
            auto* new_node = new ListNode(value, nullptr, front);
            front->prev = new_node;
            front = new_node;
        }
        size++;
    }

    void PopBack() {
        auto* new_back = back->prev;
        if (new_back != nullptr) {
            new_back->next = nullptr;
        }
        delete back;
        back = new_back;
        size--;

        if (size == 0) {
            front = nullptr;
        }
    }

    void PopFront() {
        auto* new_front = front->next;
        if (new_front != nullptr) {
            new_front->prev = nullptr;
        }
        delete front;
        front = new_front;
        size--;

        if (size == 0) {
            back = nullptr;
        }
    }

    int& Back() {
        return back->value;
    }

    int& Front() {
        return front->value;
    }

    bool IsEmpty() {
        return size == 0;
    }

    void Swap(List& other) {
        std::swap(front, other.front);
        std::swap(back, other.back);
        std::swap(size, other.size);
    }

    void Clear() {
        while (size > 0) {
            PopFront();
        }
    }

    // https://en.cppreference.com/w/cpp/container/list/splice
    // Expected behavior:
    // l1l = {1, 2, 3};
    // l1.Splice({4, 5, 6});
    // l1 == {1, 2, 3, 4, 5, 6};
    void Splice(List& other) {
        if (other.size == 0) {
            return;
        }
        if (size == 0) {
            front = other.front;
            back = other.back;
            size = other.size;
            other.front = nullptr;
            other.back = nullptr;
            other.size = 0;
        } else {
            back->next = other.front;
            other.front->prev = back;
            back = other.back;
            size += other.size;
            other.back = nullptr;
            other.front = nullptr;
            other.size = 0;
        }
    }

    template <class F>
    void ForEachElement(F&& f) const {
        ListNode* cur = front;
        while (cur != nullptr) {
            f(cur->value);
            cur = cur->next;
        }
    }

  private:
    ListNode* front;
    ListNode* back;
    size_t size;
};
