/* ============================================================
 * Lab 1 : Basic TCP Client-Server Application  --  CLIENT
 * Compile : gcc tcp_client.c -o tcp_client
 * Run     : ./tcp_client            (connects to 127.0.0.1)
 *           ./tcp_client 192.168.1.5
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

int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in servaddr;
    char sendbuf[BUFSIZE], recvbuf[BUFSIZE];
    const char *ip = (argc > 1) ? argv[1] : "127.0.0.1";
    ssize_t n;

    /* 1. Create the socket */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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

    /* 2. Connect to the server */
    if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    printf("[Client] Connected to %s:%d. Type 'exit' to quit.\n", ip, PORT);

    /* 3. Send and receive data */
    while (1) {
        printf("Enter message : ");
        if (fgets(sendbuf, BUFSIZE, stdin) == NULL) break;
        if (strncmp(sendbuf, "exit", 4) == 0) break;

        if (write(sockfd, sendbuf, strlen(sendbuf)) < 0) {
            perror("write");
            break;
        }
        if ((n = read(sockfd, recvbuf, BUFSIZE - 1)) <= 0) {
            printf("[Client] Server closed the connection.\n");
            break;
        }
        recvbuf[n] = '\0';
        printf("[Client] Echo from server : %s", recvbuf);
    }

    /* 4. Close the socket */
    close(sockfd);
    return 0;
}
