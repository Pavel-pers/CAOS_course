#include <fcntl.h>
#include <iostream>
#include <sys/wait.h>
#include <unistd.h>

void run_cmd_on_input(char* cmd, int& fd_in) {
    if (dup2(fd_in, STDIN_FILENO) == -1) {
        std::cerr << "dup2 on input failed" << std::endl;
        return;
    }

    execlp(cmd, cmd, nullptr);
    std::cerr << "exec failed" << std::endl;
}

void run_cmd_on_output(char* cmd, int& fd_out) {
    if (dup2(fd_out, STDOUT_FILENO) == -1) {
        std::cerr << "dup2 on output failed" << std::endl;
        exit(0);
    }

    execlp(cmd, cmd, nullptr);
    std::cerr << "exec failed" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: ./simple-pipeline CMD CMD";
        exit(0);
    }

    int pipes_fds[2];
    if (pipe2(pipes_fds, O_CLOEXEC) == -1) {
        std::cerr << "Pipe error";
        exit(0);
    }

    pid_t cmd_on_output_pid = fork();
    if (cmd_on_output_pid == -1) {
        std::cerr << "fork error";
        close(pipes_fds[0]);
        close(pipes_fds[1]);
        exit(0);
    }
    if (cmd_on_output_pid == 0) {
        run_cmd_on_output(argv[1], pipes_fds[1]);
        close(pipes_fds[0]);
        close(pipes_fds[1]);
        exit(0);
    }

    pid_t cmd_on_input_pid = fork();
    if (cmd_on_input_pid == -1) {
        std::cerr << "fork error";
        close(pipes_fds[0]);
        close(pipes_fds[1]);
        exit(0);
    }
    if (cmd_on_input_pid == 0) {
        run_cmd_on_input(argv[2], pipes_fds[0]);
        close(pipes_fds[0]);
        close(pipes_fds[1]);
        exit(0);
    }

    if (close(pipes_fds[0])) {
        std::cerr << "close error";
        exit(0);
    }
    if (close(pipes_fds[1])) {
        std::cerr << "close error";
        exit(0);
    }

    waitpid(cmd_on_output_pid, nullptr, 0);
    waitpid(cmd_on_input_pid, nullptr, 0);
}
