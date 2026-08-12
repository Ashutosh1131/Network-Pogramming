/* ============================================================
 * Lab 6 (a) : BLOCKING I/O model
 * The process is suspended inside read() until data arrives.
 * Compile : gcc blocking_io.c -o blocking_io
 * Run     : ./blocking_io          (then connect with tcp_client)
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    8080
#define BUFSIZE 1024

int main(void)
{
    int listenfd, connfd, opt = 1;
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
    printf("=== BLOCKING I/O === waiting on port %d\n", PORT);

    clilen = sizeof(cliaddr);
    connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
    printf("Client connected. Calling read() -- the process now BLOCKS\n");
    printf("(nothing else can be done until data actually arrives)\n");

    while ((n = read(connfd, buf, BUFSIZE - 1)) > 0) {   /* <-- blocks here */
        buf[n] = '\0';
        printf("read() returned %zd bytes : %s", n, buf);
        write(connfd, buf, n);
        printf("Blocking again in read() ...\n");
    }

    printf("Client closed the connection.\n");
    close(connfd);
    close(listenfd);
    return 0;
}
