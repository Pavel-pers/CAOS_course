#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>

extern "C" {
int __real_main(int argc, char** argv, char** env);
int __wrap_main(int argc, char** argv, char** env);
}

int __wrap_main(int argc, char** argv, char** env) {
    assert(argc == 4);
    char* prefill_str = getenv("PREFILL_SIZE");
    if (prefill_str) {
        long prefill_size = strtol(prefill_str, NULL, 10);
        int fd = open(argv[1], O_RDWR | O_CREAT, 0644);

        assert(fd >= 0);
        int truncate_ret = ftruncate(fd, prefill_size);
        assert(truncate_ret == 0);
        (void)truncate_ret;

        close(fd);
    }
    return __real_main(argc, argv, env);
}
