/* =====================================================================
 * Lab 6 (b) : NON-BLOCKING I/O model
 * The socket is put in O_NONBLOCK mode: read() returns immediately with
 * EAGAIN / EWOULDBLOCK when no data is ready, so the process polls and
 * is free to do other work between attempts.
 * Compile : gcc 2_nonblocking_io.c -o nonblocking
 * Run     : ./nonblocking    then connect with:  nc 127.0.0.1 8080
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     8080
#define BUFSIZE  1024

/* helper : add O_NONBLOCK to a descriptor's flags */
static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(void)
{
    int listenfd, connfd = -1, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buffer[BUFSIZE];
    ssize_t n;
    long polls = 0;

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

    set_nonblocking(listenfd);        /* accept() will not block either */

    printf("=== NON-BLOCKING I/O MODEL (port %d) ===\n", PORT);
    printf("Polling accept() ... (each dot = one non-blocking attempt)\n");

    /* --- poll until a client shows up --- */
    while (connfd < 0) {
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);

        if (connfd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("."); fflush(stdout);
                sleep(1);                    /* do other useful work here */
                continue;
            }
            perror("accept");
            exit(EXIT_FAILURE);
        }
    }
    printf("\nClient connected.\n");

    set_nonblocking(connfd);

    /* --- poll for data --- */
    for (;;) {
        n = read(connfd, buffer, BUFSIZE - 1);

        if (n > 0) {
            buffer[n] = '\0';
            printf("\nData ready after %ld empty polls: %s", polls, buffer);
            fflush(stdout);
            write(connfd, "ECHO (non-blocking)\n", 20);
            polls = 0;
        } else if (n == 0) {
            printf("\nClient closed the connection.\n");
            break;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                /* no data yet -- this is normal, not an error */
                polls++;
                printf("no data yet (poll #%ld)\r", polls);
                fflush(stdout);
                usleep(500000);              /* 0.5 s, then try again    */
                continue;
            }
            perror("read");
            break;
        }
    }

    close(connfd);
    close(listenfd);
    return 0;
}
