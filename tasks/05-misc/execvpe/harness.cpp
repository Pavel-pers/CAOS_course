#include "execvpe.hpp"

#include <assert.hpp>
#include <linux/limits.h>

void ParseSequence(const char* begin, const char* end, char* buf,
                   char** strbuf) {
    size_t strings = 0;
    char* last_str = buf;

    size_t pos = 0;
    auto flush = [&]() {
        buf[pos] = '\0';
        strbuf[strings++] = last_str;
        last_str = buf + pos + 1;
    };
    while (begin != end) {
        buf[pos] = *(begin++);

        if (buf[pos] == ';') {
            flush();
        }
        ++pos;
    }
    flush();
    strbuf[strings] = nullptr;
}

void HandleRun(const char* setup) {
    constexpr size_t kMaxParts = 128;

    bool is_present = setup[0] == 'p';
    ASSERT(setup[1] == ';');
    setup += 2;

    const char* border = setup;
    while (*border != '\0' && *border != '|') {
        ++border;
    }
    ASSERT(*border != '\0');

    char args_buf[PATH_MAX]{};
    char* args[kMaxParts]{};

    ParseSequence(setup, border, args_buf, args);

    auto env_start = border + 1;
    border = env_start;
    while (*border) {
        ++border;
    }

    char env_buf[PATH_MAX]{};
    char* env[kMaxParts]{};
    ParseSequence(env_start, border, env_buf, env);

    int ret = ExecVPE(args[0], const_cast<const char**>(args + 1),
                      const_cast<const char**>(env));

    if (is_present) {
        nostd::EPrint("Setup is ");
        nostd::EPrint(setup);
        nostd::EPrint("\n");
        ASSERT(false);
    }
    ASSERT(ret == -1);
}

int Main(int argc, char** argv, char**) {
    for (int i = 1; i < argc; ++i) {
        HandleRun(argv[i]);
    }
    nostd::Print("Not found\n");
    return 0;
}
