/* ============================================================
 * Lab 6 (b) : NON-BLOCKING I/O model
 * read() returns immediately with EAGAIN/EWOULDBLOCK when there
 * is no data, so the process polls in a loop and can do other work.
 * Compile : gcc nonblocking_io.c -o nonblocking_io
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    8080
#define BUFSIZE 1024

int main(void)
{
    int listenfd, connfd, flags, opt = 1;
    long polls = 0;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
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
    printf("=== NON-BLOCKING I/O === waiting on port %d\n", PORT);

    clilen = sizeof(cliaddr);
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

    /* put the connected socket into non-blocking mode */
    flags = fcntl(connfd, F_GETFL, 0);
    fcntl(connfd, F_SETFL, flags | O_NONBLOCK);
    printf("Client connected; socket set to O_NONBLOCK. Polling ...\n");

    for (;;) {
        n = read(connfd, buf, BUFSIZE - 1);

        if (n > 0) {
            buf[n] = '\0';
            printf("\nData after %ld empty polls : %s", polls, buf);
            write(connfd, buf, n);
            polls = 0;
        } else if (n == 0) {
            printf("\nClient closed the connection.\n");
            break;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
            /* no data yet -- the process is free to do other work */
            polls++;
            if (polls % 20 == 0) {
                printf("no data yet (poll #%ld) ... doing other work\r", polls);
                fflush(stdout);
            }
            usleep(100000);              /* 0.1 s, so we do not busy-spin */
        } else {
            perror("read");
            break;
        }
    }

    close(connfd);
    close(listenfd);
    return 0;
}
