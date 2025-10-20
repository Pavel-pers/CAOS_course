#include "execvpe.hpp"

#include <syscalls.hpp>

#include <c-strings.hpp>

#include <cerrno>
#include <cstddef>
#include <limits.h>

static bool is_space(char ch) {
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r' || ch == '\f' ||
           ch == '\v';
}

static const char* find_env(const char** env, const char* key) {
    if (!env) {
        return nullptr;
    }
    size_t key_len = StrLen(key);
    for (const char** it = env; *it; ++it) {
        const char* entry = *it;
        const char* cur = entry;
        while (*cur) {
            while (is_space(*cur)) {
                ++cur;
            }
            if (*cur == '\0') {
                break;
            }
            const char* start = cur;
            while (*cur && !is_space(*cur)) {
                ++cur;
            }
            if (StrNCmp(start, key, key_len) == 0 && start[key_len] == '=') {
                return start + key_len + 1;
            }
        }
    }
    return nullptr;
}

int ExecVPE(const char* file, const char** argv, const char** envp) {
    const char** run_env = envp;
    if (!run_env && Environ) {
        run_env = const_cast<const char**>(Environ);
    }

    const char** search_env = run_env;
    if (!search_env && Environ) {
        search_env = const_cast<const char**>(Environ);
    }

    const char* path = find_env(search_env, "PATH");
    if (!path) {
        path = "/usr/local/bin:/bin:/usr/bin";
    }

    bool has_slash = false;
    for (const char* p = file; *p; ++p) {
        if (*p == '/') {
            has_slash = true;
            break;
        }
    }
    if (has_slash) {
        return ExecVE(file, argv, run_env);
    }

    int last_errno = ENOENT;
    const char* segment = path;
    char buf[PATH_MAX];

    for (const char* p = path;; ++p) {
        if (*p == ':' || *p == '\0') {
            char* out = buf;
            const char* it = segment;
            while (it != p) {
                *out++ = *it++;
            }
            if (out != buf && out[-1] != '/') {
                *out++ = '/';
            }
            const char* name = file;
            while (*name) {
                *out++ = *name++;
            }
            *out = '\0';

            int rc = ExecVE(buf, argv, run_env);
            if (rc == -1) {
                if (Errno != ENOENT && Errno != ENOTDIR && Errno != EACCES) {
                    return -1;
                }
                last_errno = Errno;
            }

            if (*p == '\0') {
                Errno = last_errno;
                return -1;
            }
            segment = p + 1;
        }
    }
}
