#include <catch2/catch_session.hpp>

static int RunTestsImpl(int argc, char* argv[]) {
    return Catch::Session().run(argc, argv);
}

#ifdef __linux__

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/resource.h>
#include <sys/wait.h>
#include <type_traits>
#include <unistd.h>

struct CatchResult {
    uint64_t secret;
    int return_code;
};

static_assert(std::is_standard_layout_v<CatchResult> &&
              std::is_trivially_constructible_v<CatchResult>);

static int RunTestsForked(int argc, char* argv[]) {
    const auto kSecret = static_cast<uint64_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count());
    constexpr std::string_view kError = "\x1b[31mERROR\x1b[0m";

    struct rlimit lim;
    int r = getrlimit(RLIMIT_NOFILE, &lim);
    assert(r != -1);
    assert(lim.rlim_cur > 8);

    int fds[2];
    r = pipe2(fds, O_CLOEXEC);
    assert(r != -1);

    int child = fork();
    assert(child != -1);

    if (child == 0) {
        close(fds[0]);
        int out = dup3(fds[1], lim.rlim_cur - 1, O_CLOEXEC);
        assert(out != -1);
        close(fds[1]);

        CatchResult result{
            .secret = kSecret,
            .return_code = RunTestsImpl(argc, argv),
        };

        {
            char buf[sizeof(result)];
            memcpy(buf, &result, sizeof(buf));

            size_t pos = 0;
            while (pos < sizeof(buf)) {
                auto w = write(out, buf + pos, sizeof(buf) - pos);
                assert(w != -1);
                pos += w;
            }
        }

        return result.return_code;
    }

    close(fds[1]);
    int status;
    waitpid(child, &status, 0);

    if (WIFSIGNALED(status)) {
        raise(WTERMSIG(status));
        return 1;  // Just in case
    } else {
        assert(WIFEXITED(status));
        auto exit_code = WEXITSTATUS(status);
        if (exit_code != 0) {
            return exit_code;
        }

        CatchResult result;
        {
            char buf[sizeof(result)];
            size_t pos = 0;
            while (pos < sizeof(buf)) {
                auto r = read(fds[0], buf + pos, sizeof(buf) - pos);
                assert(r != -1);
                pos += r;

                if (r == 0) {
                    break;
                }
            }

            if (pos < sizeof(buf)) {
                std::cerr << kError
                          << " No report from catch received. Probably you've "
                             "performed exit(0) or something similar"
                          << std::endl;
                return 1;
            }
            memcpy(&result, buf, sizeof(buf));
        }

        assert(result.return_code == exit_code);
        assert(result.secret == kSecret);

        return 0;
    }
}

static bool IsCI() {
    return std::getenv("CI") != nullptr;
}

static int RunTests(int args, char* argv[]) {
    if (IsCI()) {
        std::cout << "Running Catch tests in a forked process" << std::endl;
        return RunTestsForked(args, argv);
    }
    return RunTestsImpl(args, argv);
}

#else

static int RunTests(int args, char* argv[]) {
    return RunTestsImpl(argc, argv);
}

#endif

int main(int argc, char* argv[]) {
    return RunTests(argc, argv);
}
