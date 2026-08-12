/* =====================================================================
 * Lab 7 : Signal Handling in a Network Program
 *   SIGINT  (Ctrl+C)      -> close sockets and shut down gracefully
 *   SIGCHLD (child exits) -> waitpid() in a loop, no zombies
 *   SIGPIPE (dead peer)   -> ignored, write() returns EPIPE instead
 * Compile : gcc signal_server.c -o signal_server
 * Run     : ./signal_server        (press Ctrl+C to see graceful exit)
 * Check   : ps -ef | grep defunct  -> should show no zombies
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     8080
#define BUFSIZE  1024

static int listenfd = -1;                        /* global, so SIGINT can close it */
static volatile sig_atomic_t running = 1;

/* ---------- SIGINT : graceful shutdown ---------- */
void sigint_handler(int signo)
{
    const char msg[] = "\n[SIGINT] Caught Ctrl+C -- shutting down gracefully...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);   /* write() is signal-safe */
    running = 0;
    if (listenfd >= 0) close(listenfd);
    (void)signo;
}

/* ---------- SIGCHLD : reap dead children ---------- */
void sigchld_handler(int signo)
{
    int saved_errno = errno;
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        char msg[64];
        int len = snprintf(msg, sizeof(msg), "[SIGCHLD] Reaped child PID %d\n", pid);
        write(STDOUT_FILENO, msg, len);
    }
    errno = saved_errno;
    (void)signo;
}

int main(void)
{
    int connfd;
    pid_t pid;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    struct sigaction sa_int, sa_chld;
    char buffer[BUFSIZE];
    ssize_t n;
    int opt = 1;

    /* 1. SIGINT handler -- graceful exit */
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0;                 /* NO SA_RESTART: accept() must fail  */
    sigaction(SIGINT, &sa_int, NULL);    /* with EINTR so the loop can end     */

    /* 2. SIGCHLD handler -- reap children */
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa_chld, NULL);

    /* 3. SIGPIPE -- ignore it, so writing to a closed socket returns EPIPE
          instead of killing the whole process */
    signal(SIGPIPE, SIG_IGN);

    /* 4. Ordinary socket / bind / listen */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    listen(listenfd, 10);

    printf("[SERVER %d] Listening on port %d. Handlers installed for "
           "SIGINT, SIGCHLD, SIGPIPE.\n", getpid(), PORT);
    printf("[SERVER] Press Ctrl+C to shut down gracefully.\n");

    while (running) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

        if (connfd < 0) {
            if (errno == EINTR) continue;      /* a signal interrupted us  */
            if (!running) break;               /* SIGINT closed the socket */
            perror("accept");
            continue;
        }

        printf("[SERVER] Client %s:%d connected.\n",
               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        if ((pid = fork()) == 0) {
            /* -------- child -------- */
            close(listenfd);
            signal(SIGINT, SIG_DFL);           /* child uses default SIGINT */

            while ((n = read(connfd, buffer, BUFSIZE - 1)) > 0) {
                buffer[n] = '\0';
                printf("[CHILD %d] %s", getpid(), buffer);
                fflush(stdout);

                if (write(connfd, buffer, n) < 0) {
                    if (errno == EPIPE)
                        printf("[CHILD %d] EPIPE: peer closed -- SIGPIPE was "
                               "ignored, process survived.\n", getpid());
                    else
                        perror("write");
                    break;
                }
            }
            close(connfd);
            printf("[CHILD %d] Exiting.\n", getpid());
            exit(EXIT_SUCCESS);
        }

        close(connfd);      /* parent */
    }

    printf("[SERVER] Waiting for remaining children...\n");
    while (waitpid(-1, NULL, 0) > 0)
        ;
    printf("[SERVER] All sockets closed. Clean exit.\n");
    return 0;
}
