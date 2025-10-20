#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <sys/wait.h>
#include <unistd.h>

static void usage() {
    std::fprintf(stderr, "usage:  timeout duration command [args ...]\n");
}

static bool parse_seconds(const char* text, unsigned int& value) {
    if (text == nullptr || *text == '\0') {
        return false;
    }

    char* end = nullptr;
    errno = 0;
    long parsed = std::strtol(text, &end, 10);
    if (errno != 0 || *end != '\0' || parsed < 0) {
        return false;
    }

    if (parsed > static_cast<long>(std::numeric_limits<unsigned int>::max())) {
        return false;
    }

    value = static_cast<unsigned int>(parsed);
    return true;
}

static int wait_blocking(pid_t pid, int* status) {
    while (true) {
        pid_t res = waitpid(pid, status, 0);
        if (res >= 0) {
            return 0;
        }
        if (errno == EINTR) {
            continue;
        }
        return -1;
    }
}

static int get_exit_code(int status) {
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return 125;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        usage();
        return 125;
    }

    unsigned int timeout_seconds = 0;
    if (!parse_seconds(argv[1], timeout_seconds)) {
        usage();
        return 125;
    }

    pid_t child = fork();
    if (child < 0) {
        std::perror("fork");
        return 125;
    }

    if (child == 0) {
        execvp(argv[2], argv + 2);
        std::perror("execvp");
        _exit(125);
    }

    pid_t timer = fork();
    if (timer < 0) {
        std::perror("fork");
        kill(child, SIGTERM);
        wait_blocking(child, nullptr);
        return 125;
    }

    if (timer == 0) {
        unsigned int remaining = timeout_seconds;
        while (remaining > 0) {
            remaining = sleep(remaining);
        }
        _exit(0);
    }

    int status = 0;
    pid_t finished = -1;
    while (true) {
        finished = waitpid(-1, &status, 0);
        if (finished >= 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        std::perror("waitpid");
        kill(child, SIGTERM);
        kill(timer, SIGTERM);
        wait_blocking(child, nullptr);
        wait_blocking(timer, nullptr);
        return 125;
    }

    if (finished == timer) {
        kill(child, SIGTERM);
        if (wait_blocking(child, &status) != 0) {
            std::perror("waitpid");
            return 125;
        }
        return 124;
    }

    kill(timer, SIGTERM);
    if (wait_blocking(timer, nullptr) != 0) {
        std::perror("waitpid");
        return 125;
    }

    return get_exit_code(status);
}
