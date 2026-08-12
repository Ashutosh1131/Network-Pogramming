/* ============================================================
 * Lab 6 (c) : I/O MULTIPLEXING model (select)
 * One process waits on several descriptors at once: here the
 * listening socket, the connected socket and the keyboard (stdin).
 * Compile : gcc multiplex_io.c -o multiplex_io
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    8080
#define BUFSIZE 1024

int main(void)
{
    int listenfd, connfd = -1, maxfd, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    fd_set rset;
    struct timeval tv;
    char buf[BUFSIZE];
    ssize_t n;

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    listen(listenfd, 5);
    printf("=== I/O MULTIPLEXING (select) === port %d\n", PORT);
    printf("Watching: listening socket, client socket and the keyboard.\n");

    for (;;) {
        FD_ZERO(&rset);
        FD_SET(listenfd, &rset);
        FD_SET(STDIN_FILENO, &rset);
        maxfd = listenfd;
        if (connfd >= 0) {
            FD_SET(connfd, &rset);
            if (connfd > maxfd) maxfd = connfd;
        }

        tv.tv_sec  = 5;                 /* select() can also time out */
        tv.tv_usec = 0;

        n = select(maxfd + 1, &rset, NULL, NULL, &tv);
        if (n < 0) {
            if (errno == EINTR) continue;
            perror("select"); break;
        }
        if (n == 0) { printf("select() timed out, nothing ready.\n"); continue; }

        if (FD_ISSET(listenfd, &rset)) {
            clilen = sizeof(cliaddr);
            int newfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
            if (connfd >= 0) close(connfd);      /* one client at a time */
            connfd = newfd;
            printf("Listening socket ready -> accepted %s\n",
                   inet_ntoa(cliaddr.sin_addr));
        }
        if (connfd >= 0 && FD_ISSET(connfd, &rset)) {
            if ((n = read(connfd, buf, BUFSIZE - 1)) > 0) {
                buf[n] = '\0';
                printf("Client socket ready -> %s", buf);
                write(connfd, buf, n);
            } else {
                printf("Client disconnected.\n");
                close(connfd);
                connfd = -1;
            }
        }
        if (FD_ISSET(STDIN_FILENO, &rset)) {
            if (fgets(buf, BUFSIZE, stdin) == NULL) break;
            if (strncmp(buf, "quit", 4) == 0) break;
            printf("Keyboard ready -> you typed : %s", buf);
            if (connfd >= 0) write(connfd, buf, strlen(buf));
        }
    }

    if (connfd >= 0) close(connfd);
    close(listenfd);
    return 0;
}
