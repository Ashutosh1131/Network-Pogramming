/* ============================================================
 * Lab 2 : Basic UDP Client-Server Application  --  CLIENT
 * Compile : gcc udp_client.c -o udp_client
 * Run     : ./udp_client   [server-ip]
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

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr, fromaddr;
    socklen_t fromlen;
    char sendbuf[BUFSIZE], recvbuf[BUFSIZE];
    const char *ip = (argc > 1) ? argv[1] : "127.0.0.1";
    ssize_t n;

    /* 1. Create a UDP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port   = htons(PORT);
    if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", ip);
        exit(EXIT_FAILURE);
    }

    printf("[UDP Client] Sending to %s:%d. Type 'exit' to quit.\n", ip, PORT);
    while (1) {
        printf("Enter message : ");
        if (fgets(sendbuf, BUFSIZE, stdin) == NULL) break;
        if (strncmp(sendbuf, "exit", 4) == 0) break;

        /* 2. Send the datagram directly (no connect() needed) */
        if (sendto(sockfd, sendbuf, strlen(sendbuf), 0,
                   (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
            perror("sendto");
            break;
        }

        /* 3. Wait for the server's reply */
        fromlen = sizeof(fromaddr);
        n = recvfrom(sockfd, recvbuf, BUFSIZE - 1, 0,
                     (struct sockaddr *)&fromaddr, &fromlen);
        if (n < 0) { perror("recvfrom"); break; }
        recvbuf[n] = '\0';
        printf("[UDP Client] Echo from server : %s", recvbuf);
    }

    /* 4. Close the socket */
    close(sockfd);
    return 0;
}
