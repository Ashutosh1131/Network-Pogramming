/* ============================================================
 * Lab 5 : Multithreaded TCP Server (POSIX threads)
 * Compile : gcc thread_server.c -o thread_server -pthread
 * Run     : ./thread_server
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    8080
#define BUFSIZE 1024

/* shared counter -> protected with a mutex to avoid a race condition */
static int client_count = 0;
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

/* 4. Thread handler: serves one client, then terminates */
void *handle_client(void *arg)
{
    int connfd = *(int *)arg;
    free(arg);
    char buf[BUFSIZE];
    ssize_t n;

    pthread_mutex_lock(&count_lock);
    client_count++;
    printf("[Thread %lu] Started. Active clients: %d\n",
           (unsigned long)pthread_self(), client_count);
    pthread_mutex_unlock(&count_lock);

    while ((n = read(connfd, buf, BUFSIZE - 1)) > 0) {
        buf[n] = '\0';
        printf("[Thread %lu] Received : %s", (unsigned long)pthread_self(), buf);
        write(connfd, buf, n);                  /* echo back */
    }

    close(connfd);
    pthread_mutex_lock(&count_lock);
    client_count--;
    printf("[Thread %lu] Client left. Active clients: %d\n",
           (unsigned long)pthread_self(), client_count);
    pthread_mutex_unlock(&count_lock);

    return NULL;
}

int main(void)
{
    int listenfd, opt = 1;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    pthread_t tid;

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
    printf("[Multithreaded Server] Listening on port %d ...\n", PORT);

    for (;;) {
        /* 2. Accept a client */
        clilen = sizeof(cliaddr);
        int *connfd = malloc(sizeof(int));
        if (connfd == NULL) { perror("malloc"); continue; }

        if ((*connfd = accept(listenfd,
                              (struct sockaddr *)&cliaddr, &clilen)) < 0) {
            perror("accept");
            free(connfd);
            continue;
        }
        printf("[Main] Accepted %s:%d\n",
               inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));

        /* 3. Spawn a thread for it */
        if (pthread_create(&tid, NULL, handle_client, connfd) != 0) {
            perror("pthread_create");
            close(*connfd);
            free(connfd);
            continue;
        }
        /* 6. Detach: resources are released automatically on termination */
        pthread_detach(tid);
    }
    close(listenfd);
    return 0;
}
