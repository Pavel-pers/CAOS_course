#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <string>

using ch_type = unsigned char;

bool is_cyrillic(char32_t cp) {
    return ((cp >= 0x0400 && cp <= 0x052F) || (cp >= 0x1C80 && cp <= 0x1C88) ||
            cp == 0x1D2B || (cp >= 0xA640 && cp <= 0xA66D) ||
            (cp >= 0xA680 && cp <= 0xA69B));
}

ch_type read_byte(const std::string& buf, std::size_t& pos) {
    if (pos >= buf.size()) {
        return 0;
    }
    return static_cast<ch_type>(buf[pos++]);
}

char32_t next_cp(const std::string& buf, std::size_t& pos) {
    ch_type b0 = read_byte(buf, pos);
    if ((b0 & 0x80u) == 0) {
        return b0;
    }

    if ((b0 & 0xE0u) == 0xC0u) {
        ch_type b1 = read_byte(buf, pos);
        return ((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu);
    }

    if ((b0 & 0xF0u) == 0xE0u) {
        ch_type b1 = read_byte(buf, pos);
        ch_type b2 = read_byte(buf, pos);
        char32_t cp = (b0 & 0x0Fu);
        cp = (cp << 6) | (b1 & 0x3Fu);
        cp = (cp << 6) | (b2 & 0x3Fu);
        return cp;
    }

    ch_type b1 = read_byte(buf, pos);
    ch_type b2 = read_byte(buf, pos);
    ch_type b3 = read_byte(buf, pos);
    char32_t cp = (b0 & 0x07u);
    cp = (cp << 6) | (b1 & 0x3Fu);
    cp = (cp << 6) | (b2 & 0x3Fu);
    cp = (cp << 6) | (b3 & 0x3Fu);
    return cp;
}

int main() {
    std::string input((std::istreambuf_iterator<char>(std::cin)),
                      std::istreambuf_iterator<char>());

    std::size_t total = 0, cyr = 0;

    for (std::size_t i = 0; i < input.size();) {
        char32_t cp = next_cp(input, i);
        total++;
        if (is_cyrillic(cp)) {
            cyr++;
        }
    }

    std::cout << total << ' ' << cyr;
    return 0;
}
