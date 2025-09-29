#include <iostream>

#include <sys/stat.h>
#include <sys/types.h>

bool isFileExists(const char* path) {
    struct stat buffer {};

    return (stat(path, &buffer) == 0);
}

bool isBrokenLink(const char* path) {
    struct stat link_stat {};

    if (lstat(path, &link_stat) != 0) {
        return true;
    } else {
        return false;
    }
}

int main(int argc, const char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (!isFileExists(argv[i])) {
            if (isBrokenLink(argv[i])) {
                std::cout << argv[i] << " (missing)";
            } else {
                std::cout << argv[i] << " (broken symlink)";
            }
        } else {
            std::cout << argv[i];
        }
        std::cout << '\n';
    }
}