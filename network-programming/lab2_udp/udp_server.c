/* ============================================================
 * Lab 2 : Basic UDP Client-Server Application  --  SERVER
 * Compile : gcc udp_server.c -o udp_server
 * Run     : ./udp_server
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    9090
#define BUFSIZE 1024

int main(void)
{
    int sockfd, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    ssize_t n;

    /* 1. Create a UDP (datagram) socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    /* 2. Bind it to an address and port */
    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    printf("[UDP Server] Waiting for datagrams on port %d ...\n", PORT);

    for (;;) {
        /* 3. Receive a datagram (no accept(), no connection) */
        clilen = sizeof(cliaddr);
        n = recvfrom(sockfd, buf, BUFSIZE - 1, 0,
                     (struct sockaddr *)&cliaddr, &clilen);
        if (n < 0) { perror("recvfrom"); continue; }
        buf[n] = '\0';
        printf("[UDP Server] From %s:%d -> %s",
               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port), buf);

        /* 4. Echo the datagram back to the sender */
        if (sendto(sockfd, buf, n, 0,
                   (struct sockaddr *)&cliaddr, clilen) < 0)
            perror("sendto");
    }
    close(sockfd);
    return 0;
}
