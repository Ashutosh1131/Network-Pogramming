/* ============================================================
 * Lab 3 : Concurrent TCP Server using fork()
 * Compile : gcc fork_server.c -o fork_server
 * Run     : ./fork_server      (test with several tcp_client's)
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

/* 6. Reap terminated children so that no zombie processes remain */
void sigchld_handler(int signo)
{
    int saved_errno = errno;
    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
    (void)signo;
}

int main(void)
{
    int listenfd, connfd, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    ssize_t n;
    pid_t pid;
    struct sigaction sa;

    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    /* 1. Create, bind and listen exactly as in a normal TCP server */
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
    printf("[Concurrent Server] PID %d listening on port %d ...\n",
           getpid(), PORT);

    for (;;) {
        /* 2. Accept a client connection */
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
            if (errno == EINTR) continue;       /* interrupted by SIGCHLD */
            perror("accept");
            continue;
        }

        /* 3. Fork a child process for this client */
        if ((pid = fork()) < 0) {
            perror("fork");
            close(connfd);
            continue;
        }

        if (pid == 0) {                 /* ---------- 4. CHILD ---------- */
            close(listenfd);            /* child does not need the listener */
            printf("[Child %d] Serving %s:%d\n", getpid(),
                   inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

            while ((n = read(connfd, buf, BUFSIZE - 1)) > 0) {
                buf[n] = '\0';
                printf("[Child %d] Received : %s", getpid(), buf);
                write(connfd, buf, n);
            }
            printf("[Child %d] Client disconnected. Exiting.\n", getpid());
            close(connfd);
            exit(EXIT_SUCCESS);
        }

        /* ---------------- 5. PARENT ---------------- */
        close(connfd);                  /* parent does not need this socket */
    }
    close(listenfd);
    return 0;
}
