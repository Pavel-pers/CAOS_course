#include <iostream>
#include <string>

int main() {
    std::string input;
    std::cin >> input;

    const char* s = input.c_str();
    size_t cnt_ab = 0;
state_0:
    switch (*s++) {
    case 'a':
        goto state_a;
    case '\0':
        goto finish;
    default:
        goto state_0;
    }
state_a:
    switch (*s++) {
    case 'b':
        ++cnt_ab;
        goto state_0;
    case 'a':
        goto state_a;
    case '\0':
        goto finish;
    default:
        goto state_0;
    }
finish:
    std::cout << cnt_ab << std::endl;
}
