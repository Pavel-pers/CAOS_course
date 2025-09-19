#include "reverse-list.hpp"

ListNode* Reverse(ListNode* node) {
    ListNode* prev = nullptr;
    ListNode* cur = node;
    while (cur != nullptr) {
        ListNode* next = cur->next;
        cur->next = prev;
        prev = cur;
        cur = next;
    }
    return prev;
}
