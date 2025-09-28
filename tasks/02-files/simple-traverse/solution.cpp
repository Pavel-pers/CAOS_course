#include <cstring>
#include <dirent.h>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

void pathDfs(const std::string& root_dir, const std::string& relative_path) {
    std::string full_path = root_dir + "/" + relative_path;

    DIR* directory = opendir(full_path.c_str());
    if (!directory) {
        return;
    }

    while (dirent* entry = readdir(directory)) {
        if (std::strcmp(entry->d_name, ".") == 0 ||
            std::strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        switch (entry->d_type) {
        case DT_DIR: {
            std::string next_path = relative_path + entry->d_name;
            std::cout << "d " << next_path << '/' << std::endl;
            pathDfs(root_dir, next_path + "/");
            break;
        }
        case DT_REG: {
            if (entry->d_name[0] != '.') {
                std::string file_path = relative_path + entry->d_name;
                std::cout << "f " << file_path << std::endl;
            }
            break;
        }
        case DT_LNK: {
            std::string link_path = relative_path + entry->d_name;
            std::cout << "l " << link_path << std::endl;
            break;
        }
        default: {
            break;
        }
        }
    }

    closedir(directory);
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory_path>" << std::endl;
        return 1;
    }

    pathDfs(argv[1], "");
    return 0;
}