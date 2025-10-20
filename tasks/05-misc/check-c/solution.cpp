#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

static bool write_all(int fd, const char* buf, size_t len) {
    size_t done = 0;
    while (done < len) {
        ssize_t got = write(fd, buf + done, len - done);
        if (got < 0) {
            if (errno == EINTR) continue;
            return false;
        }
        done += static_cast<size_t>(got);
    }
    return true;
}

static int run_cc(const char* path) {
    pid_t pid = fork();
    if (pid < 0) return -1;

    if (pid == 0) {
        execlp("cc", "cc", "-std=c11", "-xc", "-fsyntax-only", path, (char*)nullptr);
        _exit(1);
    }

    int st = 0;
    if (waitpid(pid, &st, 0) < 0) return -1;
    if (!WIFEXITED(st)) return -1;
    return WEXITSTATUS(st);
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::puts("Invalid");
        return 0;
    }

    char tmp[] = "/tmp/my_solutoin_cjeck-c";
    int fd = mkstemp(tmp);
    if (fd < 0) {
        std::puts("Invalid");
        return 0;
    }

    std::string body = "void __wrap(void){\n";
    body += argv[1];
    body += "\n}\n";

    bool ok = write_all(fd, body.c_str(), body.size());
    close(fd);

    int res = -1;
    if (ok) res = run_cc(tmp);
    unlink(tmp);

    if (ok && res == 0) {
        std::puts("Valid");
    } else {
        std::puts("Invalid");
    }

    return 0;
}
