/*
 * server_forked — Day 2: multi-client forking echo server
 *
 * Run:    ./server_forked [port]    (default 8080)
 * Test:   nc 127.0.0.1 8080
 *
 * Signals: SIGINT/SIGTERM trigger clean shutdown.
 * Limits:  
 *
 * WHY: Minimal baseline to compare against select/epoll in later labs.
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
#include <sys/wait.h>
#include <unistd.h>

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

static void on_sigchld(int _) { (void)_; while (waitpid(-1, NULL, WNOHANG) > 0) {} }

static void handle_client(int cfd) {
    char buf[4096];
    for (;;) {
        ssize_t n = read(cfd, buf, sizeof(buf));
        // log bytes echoed if you want this for debugging/learning
        // fprintf(stderr, "[child %d] read() -> %zd\n", getpid(), n);
        if (n == 0) {
            fprintf(stderr, "Client disconnected (fd=%d)\n", cfd);
            break;
        }
        if (n < 0) {
             if (errno == EINTR) continue; 
             perror("read"); 
             fprintf(stderr, "Client disconnected abruptly (fd=%d)\n", cfd);
             break; 
            }
        ssize_t off = 0;
        while (off < n) {
            ssize_t m = write(cfd, buf + off, n - off);
            if (m < 0) { if (errno == EINTR) continue; perror("write"); goto out; }
            off += m;
        }
    }
out:
    close(cfd);
    return;
}


int main(int argc, char **argv) {
    // For debugging, log when the server binary was built.
    fprintf(stderr, "server_forked built %s %s (pid=%d)\n", __DATE__, __TIME__, getpid()); 

    int port = (argc > 1) ? atoi(argv[1]) : 8080; // default port

    // setvbuf(stderr, NULL, _IONBF, 0); // Make stderr unbuffered for logging

    install_signals(); // setup Ctrl-C/TERM handler

    // --- install SIGCHLD handler to reap children ---
    struct sigaction sc;
    memset(&sc, 0, sizeof(sc));
    sc.sa_handler = on_sigchld;
    sigemptyset(&sc.sa_mask);
    sc.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sc, NULL);

    // opens a TCP endpoint and returns its file descriptor, 
    // lfd=listen file descriptor
    int lfd = socket(AF_INET, SOCK_STREAM, 0);  
    if (lfd < 0) { perror("socket"); return 1; } // create socket

    // NOTE: Enables quick rebind after restarts (TIME_WAIT); not multi-bind magic.
    int yes = 1;
    if (setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt"); return 1;
    }

    // bind to the given port on any/all local IPs
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    // bind the socket to the address/port
    if (bind(lfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) { 
        perror("bind"); return 1; 
    }

    // start listening for incoming connections (max 128 queued clients)
    if (listen(lfd, 128) < 0) { 
        perror("listen"); return 1; 
    }

    fprintf(stderr, "[forked] listening on :%d (Ctrl-C to stop)\n", port);

    // main loop: accept, echo, close, repeat
    while (!g_stop) {
        // accept a new client
        struct sockaddr_in cli; socklen_t clilen = sizeof(cli);

        // accept() blocks until a new client connects. cfd=client file descriptor
        int cfd = accept(lfd, (struct sockaddr*)&cli, &clilen); 

        // WHY: accept() returns EINTR if interrupted by signal (e.g. Ctrl-C).
        if (cfd < 0) {
            if (errno == EINTR) {           // interrupted by signal
                if (g_stop) break;          // exit cleanly
                continue;                   // else, retry
            }
            perror("accept");
            continue;
        }

        // log the new connection
        char ip[64]; inet_ntop(AF_INET, &cli.sin_addr, ip, sizeof(ip));
        fprintf(stderr, "client connected %s:%d\n", ip, ntohs(cli.sin_port));

        // echo loop: read until EOF, echo back what we received
         pid_t pid = fork();
        if (pid < 0) { perror("fork"); close(cfd); continue; }
        if (pid == 0) { // child
            close(lfd);
            handle_client(cfd);
            _exit(0);
        } else { // parent
            close(cfd);
        }
    }

    close(lfd);
    fprintf(stderr, "bye\n");
    return 0;
}
