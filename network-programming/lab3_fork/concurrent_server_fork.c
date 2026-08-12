/* =====================================================================
 * Lab 3 : Concurrent TCP Server using fork()
 * Compile : gcc concurrent_server_fork.c -o concurrent_server
 * Run     : ./concurrent_server
 * Test    : open 2-3 terminals and run  ../lab1_tcp/tcp_client  in each
 *           (change PORT in the client to 8081, or run tcp_client after
 *            editing this file's PORT to 8080)
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
#define BACKLOG  10

/* ---- SIGCHLD handler : reap every terminated child (no zombies) ---- */
void sigchld_handler(int signo)
{
    int saved_errno = errno;          /* waitpid() may clobber errno   */
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;                             /* loop: several may die at once */
    errno = saved_errno;
    (void)signo;
}

/* ---- the work each child does for its own client ---- */
void serve_client(int connfd, struct sockaddr_in *cliaddr)
{
    char buffer[BUFSIZE], reply[BUFSIZE + 64];
    ssize_t n;

    while ((n = read(connfd, buffer, BUFSIZE - 1)) > 0) {
        buffer[n] = '\0';
        printf("[CHILD %d] from %s:%d -> %s", getpid(),
               inet_ntoa(cliaddr->sin_addr), ntohs(cliaddr->sin_port), buffer);
        fflush(stdout);

        snprintf(reply, sizeof(reply), "[handled by PID %d] ECHO: %s",
                 getpid(), buffer);
        if (write(connfd, reply, strlen(reply)) < 0) {
            perror("write");
            break;
        }
    }
}

int main(void)
{
    int listenfd, connfd;
    pid_t pid;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    struct sigaction sa;
    int opt = 1;

    /* install the SIGCHLD handler */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;            /* restart accept() after signal */
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* 1. socket / bind / listen -- same as an ordinary TCP server */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("[PARENT %d] Concurrent server listening on port %d ...\n",
           getpid(), PORT);

    for (;;) {
        clilen = sizeof(cliaddr);

        /* 2. Accept the next client */
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
            if (errno == EINTR) continue;    /* interrupted by SIGCHLD */
            perror("accept");
            continue;
        }
        printf("[PARENT %d] New connection from %s:%d\n", getpid(),
               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        /* 3. Fork a child to handle it */
        if ((pid = fork()) < 0) {
            perror("fork");
            close(connfd);
            continue;
        }

        if (pid == 0) {
            /* ---------------- CHILD ---------------- */
            close(listenfd);                 /* child does not accept()  */
            serve_client(connfd, &cliaddr);
            close(connfd);
            printf("[CHILD %d] Client left, child exiting.\n", getpid());
            exit(EXIT_SUCCESS);
        }

        /* ---------------- PARENT ---------------- */
        close(connfd);        /* parent does not talk to this client */
    }

    close(listenfd);
    return 0;
}
