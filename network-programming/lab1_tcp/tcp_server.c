/* ============================================================
 * Lab 1 : Basic TCP Client-Server Application  --  SERVER
 * Compile : gcc tcp_server.c -o tcp_server
 * Run     : ./tcp_server
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
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

    /* 1. Create the socket */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    /* allow immediate reuse of the port after the server is restarted */
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    /* 2. Bind the socket to an address and port */
    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    /* 3. Listen for incoming connections */
    if (listen(listenfd, 5) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("[Server] Listening on port %d ...\n", PORT);

    for (;;) {
        /* 4. Accept a client connection */
        clilen = sizeof(cliaddr);
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
            perror("accept");
            continue;
        }
        printf("[Server] Connected to client %s:%d\n",
               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        /* 5. Read from / write to the client (echo service) */
        while ((n = read(connfd, buf, BUFSIZE - 1)) > 0) {
            buf[n] = '\0';
            printf("[Server] Received : %s", buf);
            if (write(connfd, buf, n) != n)
                perror("write");
        }
        if (n < 0) perror("read");

        /* 6. Close the connected socket, 7. loop back to accept() */
        printf("[Server] Client disconnected.\n");
        close(connfd);
    }
    close(listenfd);
    return 0;
}
