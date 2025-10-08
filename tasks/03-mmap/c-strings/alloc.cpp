#include "c-strings.hpp"
#include <cstdlib>

char* StrDup(const char* str) {
    const size_t n = StrLen(str);
    auto out = static_cast<char*>(std::malloc(n + 1));
    if (!out) {
        return nullptr;
    }

    char* ci = out;
    while ((*ci = *str) != '\0') {
        ci++, str++;
    }

    return out;
}

char* StrNDup(const char* str, size_t limit) {
    const size_t n = StrNLen(str, limit);
    const auto out = static_cast<char*>(std::malloc(n + 1));
    if (!out) {
        return nullptr;
    }
    for (size_t i = 0; i < n; ++i) {
        out[i] = str[i];
    }
    out[n] = '\0';
    return out;
}

char* AStrCat(const char* s1, const char* s2) {
    size_t n1 = StrLen(s1);
    size_t n2 = StrLen(s2);
    auto out = static_cast<char*>(std::malloc(n1 + n2 + 1));
    if (!out) {
        return nullptr;
    }
    for (size_t i = 0; i < n1; ++i) {
        out[i] = s1[i];
    }
    for (size_t j = 0; j < n2; ++j) {
        out[n1 + j] = s2[j];
    }
    out[n1 + n2] = '\0';
    return out;
}

void Deallocate(char* p) {
    std::free(p);
}
