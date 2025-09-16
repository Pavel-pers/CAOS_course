#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>

enum { NS_IN_SEC = 1000000000 };

static int rand_initialized;

void init_rand() {
    if (rand_initialized) {
        return;
    }
    srand(time(NULL));
    rand_initialized = 1;
}

extern "C" ssize_t __real_read(int fd, void* buf, size_t count);
extern "C" ssize_t __real_pread(int fd, void* buf, size_t count, off_t offset);
extern "C" ssize_t __real___read_chk(int fd, void* buf, size_t count, size_t buflen);
extern "C" ssize_t __real___pread_chk(int fd, void* buf, size_t count, off_t offset,
                           size_t buflen);

static void sleep_if_needed() {
    static struct timespec read_delay = {.tv_sec = 0, .tv_nsec = -1};

    if (read_delay.tv_nsec == -1) {
        const char* delay_str = getenv("READ_DELAY_NS");
        long long total_ns = 0;
        if (delay_str) {
            total_ns = strtoll(delay_str, NULL, 10);
        }
        read_delay.tv_sec = total_ns / NS_IN_SEC;
        read_delay.tv_nsec = total_ns % NS_IN_SEC;
    }

    if (read_delay.tv_sec > 0 || read_delay.tv_nsec > 0) {
        nanosleep(&read_delay, NULL);
    }
}

static unsigned read_calls;
static unsigned min_read_calls;

static void check_read_calls() {
    if (read_calls < min_read_calls) {
        fprintf(stderr, "Not enough (p)read calls\n");
        abort();
    }
}

static void count_read_call() {
    if (!min_read_calls) {
        const char* req_str = getenv("MIN_READ_CALLS");
        min_read_calls = 1;
        if (req_str) {
            min_read_calls = strtoll(req_str, NULL, 10);
        }
        atexit(check_read_calls);
    }
    ++read_calls;
}

static void tweak_read_count(size_t* count) {
    init_rand();
    if (rand() % 10 != 0 || *count == 0) {
        return;
    }
    (*count) = rand() % (*count) + 1;
}

extern "C" ssize_t __wrap_read(int fd, void* buf, size_t count) {
    count_read_call();
    sleep_if_needed();
    tweak_read_count(&count);
    return __real_read(fd, buf, count);
}

extern "C" ssize_t __wrap_pread(int fd, void* buf, size_t count, off_t offset) {
    count_read_call();
    sleep_if_needed();
    tweak_read_count(&count);
    return __real_pread(fd, buf, count, offset);
}

extern "C" ssize_t __wrap___read_chk(int fd, void* buf, size_t count, size_t buflen) {
    count_read_call();
    sleep_if_needed();
    return __real___read_chk(fd, buf, count, buflen);
}

extern "C" ssize_t __wrap___pread_chk(int fd, void* buf, size_t count, off_t offset,
                           size_t buflen) {
    count_read_call();
    sleep_if_needed();
    return __real___pread_chk(fd, buf, count, offset, buflen);
}
