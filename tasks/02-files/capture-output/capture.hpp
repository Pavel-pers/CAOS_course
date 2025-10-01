#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <variant>

inline int close_if_open(int& fd) {
    if (fd != -1) {
        if (close(fd) == -1) {
            return errno;
        }
        fd = -1;
    }
    return 0;
}

class OutputInterceptor {
  public:
    static std::variant<OutputInterceptor, int> Create(const int fd) {
        int pipe_fd[2]{-1, -1};
        const int previous_fd = dup(fd);
        if (previous_fd == -1) {
            return errno;
        }
        if (pipe(pipe_fd) == -1) {
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            close(previous_fd);
            return errno;
        }
        return OutputInterceptor(fd, previous_fd, pipe_fd);
    }

    OutputInterceptor(const OutputInterceptor&) = delete;
    OutputInterceptor& operator=(const OutputInterceptor&) = delete;

    OutputInterceptor(OutputInterceptor&& other) noexcept
        : previous_fd_(std::exchange(other.previous_fd_, -1)),
          capture_fd_(other.capture_fd_) {
        pipe_fd_[0] = std::exchange(other.pipe_fd_[0], -1);
        pipe_fd_[1] = std::exchange(other.pipe_fd_[1], -1);
    }

    OutputInterceptor& operator=(OutputInterceptor&& other) noexcept {
        if (this != &other) {
            (void)RestoreOutput();
            (void)ClosePipeRead();
            (void)ClosePipeWrite();
            previous_fd_ = std::exchange(other.previous_fd_, -1);
            pipe_fd_[0] = std::exchange(other.pipe_fd_[0], -1);
            pipe_fd_[1] = std::exchange(other.pipe_fd_[1], -1);
        }
        return *this;
    }

    [[nodiscard]] int RedirectOutput() {
        if (dup2(pipe_fd_[1], capture_fd_) == -1) {
            return errno;
        }
        return ClosePipeWrite();
    }

    [[nodiscard]] int RestoreOutput() {
        if (previous_fd_ == -1) {
            return 0;
        }
        if (dup2(previous_fd_, capture_fd_) == -1) {
            return errno;
        }
        return close_if_open(previous_fd_);
    }

    [[nodiscard]] int ClosePipeRead() {
        return close_if_open(pipe_fd_[0]);
    }

    [[nodiscard]] int ClosePipeWrite() {
        return close_if_open(pipe_fd_[1]);
    }

    [[nodiscard]] int CloseBothPipeEnds() {
        if (int rc = ClosePipeRead()) {
            return rc;
        }
        if (int rc = ClosePipeWrite()) {
            return rc;
        }
        return 0;
    }

    ~OutputInterceptor() {
        (void)RestoreOutput();
        (void)CloseBothPipeEnds();
    }

    [[nodiscard]] int Drain(std::string& output) {
        char buf[4096];
        for (;;) {
            ssize_t n = read(pipe_fd_[0], buf, sizeof(buf));
            if (n > 0) {
                output.append(buf, static_cast<size_t>(n));
            } else if (n == 0) {
                break;
            } else if (errno == EINTR) {
                continue;
            } else {
                return errno;
            }
        }
        return ClosePipeRead();
    }

  private:
    explicit OutputInterceptor(const int capture_fd, const int copy_fd,
                               const int pipe_fd[])
        : previous_fd_(copy_fd), capture_fd_(capture_fd) {
        pipe_fd_[0] = pipe_fd[0];
        pipe_fd_[1] = pipe_fd[1];
    }

    int pipe_fd_[2]{-1, -1};
    int previous_fd_{-1};
    const int capture_fd_;
};

class InputInterceptor {
  public:
    static std::variant<InputInterceptor, int> Create(const int fd) {
        int pipe_fd[2]{-1, -1};
        const int previous_fd = dup(fd);
        if (previous_fd == -1) {
            return errno;
        }
        if (pipe(pipe_fd) == -1) {
            close(pipe_fd[0]);
            close(pipe_fd[1]);
            close(previous_fd);
            return errno;
        }
        return InputInterceptor(fd, previous_fd, pipe_fd);
    }

    InputInterceptor(const InputInterceptor&) = delete;
    InputInterceptor& operator=(const InputInterceptor&) = delete;

    InputInterceptor(InputInterceptor&& other) noexcept
        : previous_fd_(std::exchange(other.previous_fd_, -1)),
          capture_fd_(other.capture_fd_) {
        pipe_fd_[0] = std::exchange(other.pipe_fd_[0], -1);
        pipe_fd_[1] = std::exchange(other.pipe_fd_[1], -1);
    }

    InputInterceptor& operator=(InputInterceptor&& other) noexcept {
        if (this != &other) {
            (void)RestoreInput();
            (void)ClosePipeWrite();
            (void)ClosePipeRead();
            previous_fd_ = std::exchange(other.previous_fd_, -1);
            pipe_fd_[0] = std::exchange(other.pipe_fd_[0], -1);
            pipe_fd_[1] = std::exchange(other.pipe_fd_[1], -1);
        }
        return *this;
    }

    [[nodiscard]] int RedirectInput() {
        if (dup2(pipe_fd_[0], capture_fd_) == -1) {
            return errno;
        }
        return ClosePipeRead();
    }

    [[nodiscard]] int RestoreInput() {
        if (previous_fd_ == -1) {
            return 0;
        }
        if (dup2(previous_fd_, capture_fd_) == -1) {
            return errno;
        }
        return close_if_open(previous_fd_);
    }

    [[nodiscard]] int ClosePipeWrite() {
        return close_if_open(pipe_fd_[1]);
    }

    [[nodiscard]] int ClosePipeRead() {
        return close_if_open(pipe_fd_[0]);
    }

    [[nodiscard]] int CloseBothPipeEnds() {
        if (int rc = ClosePipeWrite()) {
            return rc;
        }
        if (int rc = ClosePipeRead()) {
            return rc;
        }
        return 0;
    }

    ~InputInterceptor() {
        (void)RestoreInput();
        (void)CloseBothPipeEnds();
    }

    [[nodiscard]] int WriteToInput(std::string_view input) {
        while (!input.empty()) {
            ssize_t n = write(pipe_fd_[1], input.data(), input.size());
            if (n > 0) {
                input.remove_prefix(static_cast<size_t>(n));
            } else if (n == -1 && errno == EINTR) {
                continue;
            } else {
                return errno;
            }
        }
        return ClosePipeWrite();
    }

  private:
    explicit InputInterceptor(const int capture_fd, const int copy_fd,
                              const int pipe_fd[])
        : previous_fd_(copy_fd), capture_fd_(capture_fd) {
        pipe_fd_[0] = pipe_fd[0];
        pipe_fd_[1] = pipe_fd[1];
    }

    int pipe_fd_[2]{-1, -1};
    int previous_fd_{-1};
    const int capture_fd_;
};

template <class F>
std::variant<std::pair<std::string, std::string>, int>
CaptureOutput(F&& f, const std::string_view input) {
    auto in_m = InputInterceptor::Create(STDIN_FILENO);
    if (std::holds_alternative<int>(in_m)) {
        return std::get<int>(in_m);
    }
    auto out_m = OutputInterceptor::Create(STDOUT_FILENO);
    if (std::holds_alternative<int>(out_m)) {
        return std::get<int>(out_m);
    }
    auto err_m = OutputInterceptor::Create(STDERR_FILENO);
    if (std::holds_alternative<int>(err_m)) {
        return std::get<int>(err_m);
    }

    auto& input_di = std::get<InputInterceptor>(in_m);
    auto& output_di = std::get<OutputInterceptor>(out_m);
    auto& errput_di = std::get<OutputInterceptor>(err_m);

    if (int rc = input_di.RedirectInput()) {
        return rc;
    }
    if (int rc = output_di.RedirectOutput()) {
        return rc;
    }
    if (int rc = errput_di.RedirectOutput()) {
        return rc;
    }

    if (int rc = input_di.WriteToInput(input)) {
        return rc;
    }

    f();

    std::cout.flush();
    std::cerr.flush();
    std::fflush(nullptr);

    if (int rc = output_di.RestoreOutput()) {
        return rc;
    }
    if (int rc = errput_di.RestoreOutput()) {
        return rc;
    }

    std::string out, err;
    if (int rc = output_di.Drain(out)) {
        return rc;
    }
    if (int rc = errput_di.Drain(err)) {
        return rc;
    }

    return std::make_pair(std::move(out), std::move(err));
}
