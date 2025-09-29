#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>
#include <utility>
#include <variant>

class OutputDescriptorInterceptor {
  public:
    explicit OutputDescriptorInterceptor(const int fd) {
        capture_fd_ = fd;
        previous_fd_ = dup(fd);
        pipe2(pipe_fd_, 0);
    }

    [[nodiscard]] int RedirectOutput() const {
        if (dup2(pipe_fd_[1], capture_fd_) == -1) {
            return errno;
        }
        return 0;
    }

    [[nodiscard]] int RestoreOutput() {
        if (dup2(previous_fd_, capture_fd_) == -1) {
            return errno;
        }

        if (pipe_fd_[1] != -1) {
            if (close(pipe_fd_[1])) {
                return errno;
            }
            pipe_fd_[1] = -1;
        }
        return 0;
    }

    [[nodiscard]] int CloseOutput() {
        if (pipe_fd_[0] != -1) {
            if (close(pipe_fd_[0])) {
                return errno;
            }
            pipe_fd_[0] = -1;
        }
        return 0;
    }

    ~OutputDescriptorInterceptor() {
        (void)RestoreOutput();
        (void)CloseOutput();
    }

    [[nodiscard]] int ReadOutput(std::string& output) {
        if (const int rc = RestoreOutput()) {
            return rc;
        }

        char buf[4096];
        ssize_t bytes_read;

        while ((bytes_read = read(pipe_fd_[0], buf, sizeof(buf))) > 0) {
            output.append(buf, bytes_read);
        }

        if (bytes_read < 0) {
            return errno;
        }
        if (const int rc = CloseOutput()) {
            return rc;
        }
        return 0;
    }

  private:
    int pipe_fd_[2]{-1, -1};
    int previous_fd_;
    int capture_fd_;
};

class InputDescriptorsInterceptor {
  public:
    explicit InputDescriptorsInterceptor(const int fd) {
        capture_fd_ = fd;
        previous_fd_ = dup(fd);
        pipe2(pipe_fd_, 0);
    }

    [[nodiscard]] int RedirectInput() const {
        if (dup2(pipe_fd_[0], capture_fd_) == -1) {
            return errno;
        }
        return 0;
    }

    [[nodiscard]] int RestoreInput() {
        if (dup2(previous_fd_, capture_fd_) == -1) {
            return errno;
        }

        if (pipe_fd_[0] != -1) {
            if (close(pipe_fd_[0])) {
                return errno;
            }
            pipe_fd_[0] = -1;
        }
        return 0;
    }

    [[nodiscard]] int CloseInput() {
        if (pipe_fd_[1] != -1) {
            if (close(pipe_fd_[1])) {
                return errno;
            }
            pipe_fd_[1] = -1;
        }
        return 0;
    }

    ~InputDescriptorsInterceptor() {
        (void)RestoreInput();
        (void)CloseInput();
    }

    [[nodiscard]] int WriteToInput(std::string_view input) {
        while (!input.empty()) {
            ssize_t written = write(pipe_fd_[1], input.data(), input.size());
            if (written < 0) {
                return errno;
            }
            input.remove_prefix(static_cast<size_t>(written));
        }

        if (const int rc = CloseInput()) {
            return rc;
        }
        return 0;
    }

  private:
    int pipe_fd_[2]{-1, -1};
    int previous_fd_;
    int capture_fd_;
};

template <class F>
std::variant<std::pair<std::string, std::string>, int>
CaptureOutput(F&& f, const std::string_view input) {
    InputDescriptorsInterceptor input_di(0);
    OutputDescriptorInterceptor output_di(1);
    OutputDescriptorInterceptor errput_di(2);

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

    std::string out, err;
    if (int rc = output_di.ReadOutput(out)) {
        return rc;
    }
    if (int rc = errput_di.ReadOutput(err)) {
        return rc;
    }

    return std::make_pair(std::move(out), std::move(err));
}