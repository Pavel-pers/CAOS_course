#include <nejudge/checker2.h>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

void resolve(const char* inbuf, char* corrbuf, size_t corrsize) {
    char node[1024], service[1024];

    if (sscanf(inbuf, "%s%s", node, service) != 2) {
        fatal_CF("failed to parse input line");
    }
    struct addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* result = NULL;
    int err = getaddrinfo(node, service, &hints, &result);
    if (err) {
        snprintf(corrbuf, corrsize, "%s", gai_strerror(err));
    } else {
        if (result) {
            const struct sockaddr_in* minp = NULL;
            for (struct addrinfo* p = result; p; p = p->ai_next) {
                const struct sockaddr_in* addr =
                    (const struct sockaddr_in*)p->ai_addr;
                if (!minp) {
                    minp = addr;
                } else if (ntohl(addr->sin_addr.s_addr) <
                           ntohl(minp->sin_addr.s_addr)) {
                    minp = addr;
                }
            }
            if (minp) {
                snprintf(corrbuf, corrsize, "%s:%d", inet_ntoa(minp->sin_addr),
                         ntohs(minp->sin_port));
            }
        }
        freeaddrinfo(result);
    }
}

/*
  argv[1] - input
  argv[2] - output
  argv[3] - correct
 */
int main(int argc, char* argv[]) {
    if (argc != 5) {
        fatal_CF("wrong number of arguments");
    }
    FILE* fin = fopen(argv[1], "r");
    if (!fin) {
        fatal_CF("cannot open test input file '%s'", argv[1]);
    }

    int fdout = open(argv[2], O_RDONLY | O_NONBLOCK | O_NOFOLLOW, 0);
    if (fdout < 0) {
        fatal_CF("cannot open program output file '%s': %s", argv[2],
                 strerror(errno));
    }
    fcntl(fdout, F_SETFL, fcntl(0, F_GETFL) & ~O_NONBLOCK);
    FILE* fout = fdopen(fdout, "r");
    fdout = -1;

    int lineno = 0;
    char inbuf[1024];
    char corrbuf[1024];
    char outbuf[1024];
    while (fgets(inbuf, sizeof(inbuf), fin)) {
        int inlen = strlen(inbuf);
        if (inlen > 900) {
            fatal_CF("input line '%s' is too long: %d\n", inbuf, inlen);
        }
        while (inlen > 0 && isspace(inbuf[inlen - 1])) {
            --inlen;
        }
        inbuf[inlen] = 0;
        if (inlen <= 0) {
            fatal_CF("empty input line");
        }
        resolve(inbuf, corrbuf, sizeof(corrbuf));
        ++lineno;
        if (!fgets(outbuf, sizeof(outbuf), fout)) {
            fatal_PE("unexpected EOF in program output file");
        }
        int outlen = strlen(outbuf);
        if (outlen > 900) {
            fatal_PE("output line %d is too long: %d\n", lineno, outlen);
        }
        while (outlen > 0 && isspace(outbuf[outlen - 1])) {
            --outlen;
        }
        outbuf[outlen] = 0;
        if (outlen <= 0) {
            fatal_PE("empty output line %d", lineno);
        }
        if (strcmp(corrbuf, outbuf) != 0) {
            fatal_WA("wrong answer '%s', correct is '%s'", outbuf, corrbuf);
        }
    }

    while (fgets(outbuf, sizeof(outbuf), fout)) {
        ++lineno;
        int outlen = strlen(outbuf);
        if (outlen > 900) {
            fatal_PE("output line %d is too long: %d\n", lineno, outlen);
        }
        while (outlen > 0 && isspace(outbuf[outlen - 1])) {
            --outlen;
        }
        outbuf[outlen] = 0;
        if (outlen > 0) {
            fatal_PE("garbage in output line %d", lineno);
        }
    }
}
