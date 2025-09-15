/*
 * server_blocking.c — Day 1: single-client blocking echo server
 *
 * Run:    ./server_blocking [port]    (default 8080)
 * Test:   nc 127.0.0.1 8080
 *
 * Signals: SIGINT/SIGTERM trigger clean shutdown.
 * Limits:  Accepts one client at a time; next client blocks at accept().
 *
 * WHY: Minimal baseline to compare against forked/select/epoll in later labs.
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

// Use this to experiment and log various things to help learning. Feel free to
// add more logs as you see fit.
#define LOG(fmt, ...) fprintf(stderr, "[blocking] " fmt "\n", ##__VA_ARGS__)

static volatile sig_atomic_t g_stop = 0;

// WHY: Use sigaction *without* SA_RESTART so blocking syscalls return EINTR.
//      This lets the main loop notice g_stop promptly on Ctrl-C/TERM.
static void on_stop(int _) { 
    (void)_; g_stop = 1; 
}

static void install_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;                // <-- no SA_RESTART
    sigaction(SIGINT,  &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);  // allow `kill -TERM` too
}
int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 8080;

    install_signals();

    int lfd = socket(AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) { perror("socket"); return 1; }

    // NOTE: Enables quick rebind after restarts (TIME_WAIT); not multi-bind magic.
    int yes = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt"); return 1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { 
        perror("bind"); return 1; 
    }

    if (listen(lfd, 128) < 0) { 
        perror("listen"); return 1; 
    }

    fprintf(stderr, "[blocking] listening on :%d (Ctrl-C to stop)\n", port);

    while (!g_stop) {
        struct sockaddr_in cli; socklen_t clilen = sizeof(cli);

        int cfd = accept(lfd, (struct sockaddr*)&cli, &clilen);
if (cfd < 0) {
    if (errno == EINTR) {           // interrupted by signal
        if (g_stop) break;          // exit cleanly
        continue;                   // else, retry
    }
    perror("accept");
    continue;
}


        char ip[64]; inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        fprintf(stderr, "client connected %s:%d\n", ip, ntohs(cli.sin_port));

        char buf[4096];
        for (;;) {
            ssize_t n = read(cfd, buf, sizeof(buf));
            if (n == 0) break;                 // client closed
            if (n < 0) {
                if (errno == EINTR) continue;     // retry short-term interrupts
                perror("read"); break;
            }
            // PERF: write() may be partial; loop until we've echoed all n bytes.
            ssize_t off = 0;
            while (off < n) {
                ssize_t m = write(cfd, buf + off, n - off);
                if (m < 0) { 
                    if (errno == EINTR) continue; 
                    perror("write"); 
                    goto done; 
                }
                off += m;
            }
        }
    done:
        close(cfd);      // ownership of cfd ends here
        fprintf(stderr, "client disconnected\n");
    }

    close(lfd);
    fprintf(stderr, "bye\n");
    return 0;
}
