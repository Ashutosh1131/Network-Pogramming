/* ============================================================
 * Lab 7 : Signal Handling in a Network Program
 *   SIGINT  -> graceful shutdown (close sockets, then exit)
 *   SIGCHLD -> reap terminated children (no zombies)
 *   SIGPIPE -> ignored, so write() to a closed peer returns EPIPE
 * Compile : gcc signal_server.c -o signal_server
 * Run     : ./signal_server        (press Ctrl+C to stop)
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    8080
#define BUFSIZE 1024

static volatile sig_atomic_t running = 1;
static int listenfd = -1;

/* 1. SIGINT (Ctrl+C) -> ask the main loop to stop */
void sigint_handler(int signo)
{
    (void)signo;
    running = 0;
    if (listenfd >= 0) close(listenfd);   /* makes accept() fail immediately */
    const char *msg = "\n[SIGINT] Shutting down gracefully ...\n";
    write(STDOUT_FILENO, msg, strlen(msg));
}

/* 2. SIGCHLD -> reap every terminated child */
void sigchld_handler(int signo)
{
    (void)signo;
    int saved_errno = errno;
    pid_t pid;
    while ((pid = waitpid(-1, NULL, WNOHANG)) > 0) {
        char msg[64];
        int len = snprintf(msg, sizeof(msg),
                           "[SIGCHLD] Reaped child %d\n", (int)pid);
        write(STDOUT_FILENO, msg, len);
    }
    errno = saved_errno;
}

int main(void)
{
    int connfd, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    ssize_t n;
    struct sigaction sa;

    /* register SIGINT (no SA_RESTART, so a blocked accept() is interrupted) */
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    /* register SIGCHLD */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    /* 3. ignore SIGPIPE -> write() returns -1/EPIPE instead of killing us */
    signal(SIGPIPE, SIG_IGN);

    /* 4. usual TCP server setup */
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
    if (listen(listenfd, 10) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }
    printf("[Server] PID %d listening on port %d (Ctrl+C to stop)\n",
           getpid(), PORT);

    /* 5. accept clients, forking a child for each one */
    while (running) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
        if (connfd < 0) {
            if (!running) break;              /* SIGINT closed the listener */
            if (errno == EINTR) continue;     /* some other signal arrived  */
            perror("accept");
            continue;
        }

        if (fork() == 0) {                    /* ---------- child ---------- */
            close(listenfd);
            printf("[Child %d] Serving %s\n", getpid(),
                   inet_ntoa(cliaddr.sin_addr));
            while ((n = read(connfd, buf, BUFSIZE - 1)) > 0) {
                buf[n] = '\0';
                printf("[Child %d] Received : %s", getpid(), buf);
                if (write(connfd, buf, n) < 0) {
                    if (errno == EPIPE)
                        printf("[Child %d] EPIPE: peer closed the socket "
                               "(SIGPIPE was ignored, process survives)\n",
                               getpid());
                    else
                        perror("write");
                    break;
                }
            }
            printf("[Child %d] Done, exiting.\n", getpid());
            close(connfd);
            exit(EXIT_SUCCESS);
        }
        close(connfd);                        /* --------- parent ---------- */
    }

    printf("[Server] Listening socket closed. Exiting cleanly.\n");
    return 0;
}
