/* ============================================================
 * Lab 4 : Handling Multiple Descriptors using select()
 * Compile : gcc select_server.c -o select_server
 * Run     : ./select_server     (connect several tcp_client's)
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

#define PORT       8080
#define BUFSIZE    1024
#define MAXCLIENTS FD_SETSIZE

int main(void)
{
    int listenfd, connfd, maxfd, i, opt = 1;
    int client[MAXCLIENTS];                 /* -1 == free slot */
    fd_set allset, rset;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    ssize_t n;

    /* 1. Create, bind and listen */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }
    if (listen(listenfd, 10) < 0) {
        perror("listen"); exit(EXIT_FAILURE);
    }
    printf("[select Server] Listening on port %d ...\n", PORT);

    /* 2. Initialise the descriptor set with the listening socket */
    for (i = 0; i < MAXCLIENTS; i++) client[i] = -1;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);
    maxfd = listenfd;

    for (;;) {
        rset = allset;                      /* select() modifies the set */

        /* 3. Block until one or more descriptors are readable */
        if (select(maxfd + 1, &rset, NULL, NULL, NULL) < 0) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }

        /* 4. A new connection is pending on the listening socket */
        if (FD_ISSET(listenfd, &rset)) {
            clilen = sizeof(cliaddr);
            if ((connfd = accept(listenfd,
                                 (struct sockaddr *)&cliaddr, &clilen)) >= 0) {
                for (i = 0; i < MAXCLIENTS; i++)
                    if (client[i] < 0) { client[i] = connfd; break; }

                if (i == MAXCLIENTS) {
                    fprintf(stderr, "Too many clients\n");
                    close(connfd);
                } else {
                    FD_SET(connfd, &allset);
                    if (connfd > maxfd) maxfd = connfd;
                    printf("[select Server] New client %s:%d on fd %d\n",
                           inet_ntoa(cliaddr.sin_addr),
                           ntohs(cliaddr.sin_port), connfd);
                }
            }
        }

        /* 5./6. Check every connected client for readable data */
        for (i = 0; i < MAXCLIENTS; i++) {
            int fd = client[i];
            if (fd < 0) continue;
            if (!FD_ISSET(fd, &rset)) continue;

            if ((n = read(fd, buf, BUFSIZE - 1)) > 0) {
                buf[n] = '\0';
                printf("[select Server] fd %d says : %s", fd, buf);
                write(fd, buf, n);              /* echo back */
            } else {                            /* client closed / error */
                printf("[select Server] fd %d disconnected\n", fd);
                close(fd);
                FD_CLR(fd, &allset);
                client[i] = -1;
            }
        }
    }
    close(listenfd);
    return 0;
}
