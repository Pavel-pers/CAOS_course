#include <iostream>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <unistd.h>

void run_cmd(char* cmd, int fd_in, int fd_out) {
    if (dup2(fd_in, STDIN_FILENO) == -1) {
        std::cerr << "dup2 failed" << std::endl;
        return;
    }
    if (dup2(fd_out, STDOUT_FILENO) == -1) {
        std::cerr << "dup2 failed" << std::endl;
        return;
    }

    execlp(cmd, cmd, nullptr);
    std::cerr << "exec failed" << std::endl;
}

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "Usage: ./redirect-io CMD input_file output_file"
                  << std::endl;
        exit(1);
    }
    char* cmd = argv[1];
    char* in_file = argv[2];
    char* out_file = argv[3];
    int fd_inp = open(in_file, O_RDONLY | O_CLOEXEC);
    if (fd_inp == -1) {
        std::cerr << "open failed" << std::endl;
        exit(1);
    }
    int fd_out = open(out_file, O_WRONLY | O_CREAT | O_CLOEXEC, 0666);
    if (fd_out == -1) {
        std::cerr << "open failed" << std::endl;
        exit(1);
    }

    pid_t child_pid = fork();
    if (child_pid == 0) {
        run_cmd(cmd, fd_inp, fd_out);
    }
    waitpid(child_pid, nullptr, 0);
    close(fd_inp);
    close(fd_out);
}
