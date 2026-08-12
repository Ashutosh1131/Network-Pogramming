/* =====================================================================
 * Lab 6 (a) : BLOCKING I/O model
 * The process is suspended inside read() until data actually arrives.
 * Compile : gcc 1_blocking_io.c -o blocking
 * Run     : ./blocking     then connect with:  nc 127.0.0.1 8080
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     8080
#define BUFSIZE  1024

static void stamp(const char *msg)
{
    time_t t = time(NULL);
    struct tm *lt = localtime(&t);
    printf("[%02d:%02d:%02d] %s\n", lt->tm_hour, lt->tm_min, lt->tm_sec, msg);
    fflush(stdout);
}

int main(void)
{
    int listenfd, connfd, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buffer[BUFSIZE];
    ssize_t n;

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

    printf("=== BLOCKING I/O MODEL (port %d) ===\n", PORT);
    stamp("Calling accept() -- process BLOCKS here until a client connects");

    clilen = sizeof(cliaddr);
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    if (connfd < 0) { perror("accept"); exit(EXIT_FAILURE); }
    stamp("accept() returned -- client connected");

    for (;;) {
        stamp("Calling read() -- process BLOCKS here until data arrives");

        n = read(connfd, buffer, BUFSIZE - 1);   /* <-- the blocking call */

        if (n <= 0) { stamp("Client closed the connection"); break; }

        buffer[n] = '\0';
        printf("           read() returned %zd bytes: %s", n, buffer);
        fflush(stdout);
        write(connfd, "ECHO (blocking)\n", 16);
    }

    close(connfd);
    close(listenfd);
    return 0;
}
