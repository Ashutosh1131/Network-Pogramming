/* =====================================================================
 * Lab 6 (c) : I/O MULTIPLEXING model  (select())
 * One process waits on SEVERAL descriptors at once -- here the listening
 * socket, the connected client socket and the keyboard (stdin).
 * Compile : gcc 3_multiplexing_io.c -o multiplexing
 * Run     : ./multiplexing   then connect with:  nc 127.0.0.1 8080
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     8080
#define BUFSIZE  1024

int main(void)
{
    int listenfd, connfd = -1, maxfd, nready, opt = 1;
    fd_set rset;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buffer[BUFSIZE];
    ssize_t n;
    struct timeval tv;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) { perror("socket"); exit(EXIT_FAILURE); }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    listen(listenfd, 5);

    printf("=== I/O MULTIPLEXING MODEL (port %d) ===\n", PORT);
    printf("select() is watching: listening socket, client socket, keyboard.\n");
    printf("Type here to send a line to the connected client.\n");

    for (;;) {
        /* rebuild the descriptor set before every select() call */
        FD_ZERO(&rset);
        FD_SET(listenfd,      &rset);
        FD_SET(STDIN_FILENO,  &rset);
        maxfd = listenfd;

        if (connfd >= 0) {
            FD_SET(connfd, &rset);
            if (connfd > maxfd) maxfd = connfd;
        }

        tv.tv_sec  = 5;      /* 5-second timeout, just to show it works */
        tv.tv_usec = 0;

        nready = select(maxfd + 1, &rset, NULL, NULL, &tv);

        if (nready < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }
        if (nready == 0) {
            printf("select() timed out -- nothing ready, still waiting.\n");
            continue;
        }

        /* --- the listening socket is ready => a new connection --- */
        if (FD_ISSET(listenfd, &rset)) {
            clilen = sizeof(cliaddr);
            int newfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
            if (newfd < 0) { perror("accept"); continue; }

            if (connfd >= 0) {          /* keep this demo to one client */
                write(newfd, "Busy: one client at a time\n", 27);
                close(newfd);
            } else {
                connfd = newfd;
                printf("New client %s:%d (fd %d)\n",
                       inet_ntoa(cliaddr.sin_addr),
                       ntohs(cliaddr.sin_port), connfd);
            }
        }

        /* --- the client socket is ready => data to read --- */
        if (connfd >= 0 && FD_ISSET(connfd, &rset)) {
            n = read(connfd, buffer, BUFSIZE - 1);
            if (n <= 0) {
                printf("Client disconnected.\n");
                close(connfd);
                connfd = -1;
            } else {
                buffer[n] = '\0';
                printf("From client: %s", buffer);
                write(connfd, "ECHO (select)\n", 14);
            }
        }

        /* --- the keyboard is ready => forward the line to the client --- */
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            if (fgets(buffer, BUFSIZE, stdin) == NULL) break;
            if (connfd >= 0)
                write(connfd, buffer, strlen(buffer));
            else
                printf("(no client connected yet)\n");
        }
    }

    if (connfd >= 0) close(connfd);
    close(listenfd);
    return 0;
}
