#include "count-substrings.h"

SubstringsCount CountSubstrings(const char* s) {
    SubstringsCount res{};
    if (s == nullptr) {
        return res;
    }

    const char* it = s;
    char ch = '\0';

    goto S0;

S0:
    ch = *it++;
    if (ch == '\0') goto END;
    if (ch == 'o') goto So;
    if (ch == 'r') goto Sr;
    goto S0;

So:
    ch = *it++;
    if (ch == '\0') goto END;
    if (ch == 's') {
        ++res.os_num;
        goto S0;
    }
    if (ch == 'o') goto So;
    if (ch == 'r') goto Sr;
    goto S0;

Sr:
    ch = *it++;
    if (ch == '\0') goto END;
    if (ch == 'o') goto Sro;
    if (ch == 'r') goto Sr;
    goto S0;

Sro:
    ch = *it++;
    if (ch == '\0') goto END;
    if (ch == 'p') {
        ++res.rop_num;
        goto S0;
    }
    if (ch == 's') {
        ++res.os_num;
        goto S0;
    }
    if (ch == 'o') goto So;
    if (ch == 'r') goto Sr;
    goto S0;

END:
    return res;
}
