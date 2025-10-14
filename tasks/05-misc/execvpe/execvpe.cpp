#include "execvpe.hpp"

#include <syscalls.hpp>

#include <c-strings.hpp>

#include <cerrno>          // ENOENT ENOTDIR EACCES
#include <linux/limits.h>  // PATH_MAX

#include <unused.hpp>  // TODO: remove before flight.

int ExecVPE(const char* file, const char** argv, const char** envp) {
    UNUSED(file, argv, envp);  // TODO: remove before flight.
    return 1;  // TODO: remove before flight.
}
