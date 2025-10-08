#include <cstddef>

extern "C" void* memset(void* dst, int c, size_t n) {
    unsigned char* p = (unsigned char*)dst;
    unsigned char v = (unsigned char)c;
    for (size_t i = 0; i < n; ++i) {
        p[i] = v;
    }
    return dst;
}

extern "C" void* memcpy(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    for (size_t i = 0; i < n; ++i) {
        *(d++) = *(s++);
    }
    return dst;
}

extern "C" void* memmove(void* dst, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dst;
    const unsigned char* s = (const unsigned char*)src;
    if (d < s) {
        for (size_t i = 0; i < n; ++i) {
            d[i] = s[i];
        }
    } else {
        for (size_t i = n; i-- > 0;) {
            d[i] = s[i];
        }
    }
    return dst;
}

extern "C" int memcmp(const void* a, const void* b, size_t n) {
    const unsigned char* p = (const unsigned char*)a;
    const unsigned char* q = (const unsigned char*)b;
    for (size_t i = 0; i < n; ++i) {
        int diff = (int)p[i] - (int)q[i];
        if (diff != 0) {
            return diff;
        }
    }
    return 0;
}
