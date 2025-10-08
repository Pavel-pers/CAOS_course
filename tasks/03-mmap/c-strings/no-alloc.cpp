#include "c-strings.hpp"

#include <unused.hpp>  // TODO: remove before flight.

size_t StrLen(const char* str) {
    const char* end = str;
    while (*end) {
        end++;
    }
    return static_cast<size_t>(end - str);
}

size_t StrNLen(const char* str, size_t limit) {
    const char* end = str;
    while (*end && static_cast<size_t>(end - str) < limit) {
        end++;
    }
    return static_cast<size_t>(end - str);
}

int StrCmp(const char* lhs, const char* rhs) {
    while (*lhs == *rhs) {
        if (*lhs == '\0') {
            return 0;
        }
        lhs++, rhs++;
    }
    return static_cast<int>(*lhs) - static_cast<int>(*rhs);
}

int StrNCmp(const char* lhs, const char* rhs, size_t limit) {
    while (limit--) {
        if (*lhs != *rhs) {
            return static_cast<int>(*lhs) - static_cast<int>(*rhs);
        }
        if (*lhs == '\0') {
            return 0;
        }
        lhs++, rhs++;
    }
    return 0;
}

char* StrCat(char* s1, const char* s2) {
    char* p = s1 + StrLen(s1);
    while ((*p++ = *s2++) != '\0') {
    }
    return s1;
}

const char* StrStr(const char* haystack, const char* needle) {
    if (*needle == '\0') {
        return haystack;
    }
    size_t needle_len = StrLen(needle);
    for (int i = 0; haystack[i] != '\0'; i++) {
        if (StrNCmp(haystack + i, needle, needle_len) == 0) {
            return haystack + i;
        }
    }
    return nullptr;
}
