/* =====================================================================
 * Lab 5 : Multithreaded TCP Server (POSIX threads)
 * Compile : gcc multithreaded_server.c -o mt_server -lpthread
 * Run     : ./mt_server
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     8080
#define BUFSIZE  1024
#define BACKLOG  10

/* shared counter -- protected by a mutex because threads share memory */
static int client_count = 0;
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

/* argument passed to each thread */
typedef struct {
    int connfd;
    struct sockaddr_in cliaddr;
} client_arg_t;

/* ---- thread routine : handles exactly one client ---- */
void *handle_client(void *arg)
{
    client_arg_t *ca = (client_arg_t *)arg;
    int connfd = ca->connfd;
    char ip[INET_ADDRSTRLEN];
    char buffer[BUFSIZE], reply[BUFSIZE + 64];
    ssize_t n;
    int my_id;

    inet_ntop(AF_INET, &ca->cliaddr.sin_addr, ip, sizeof(ip));

    pthread_mutex_lock(&count_lock);
    my_id = ++client_count;
    pthread_mutex_unlock(&count_lock);

    printf("[THREAD %lu] Serving client #%d (%s:%d)\n",
           (unsigned long)pthread_self(), my_id, ip, ntohs(ca->cliaddr.sin_port));

    while ((n = read(connfd, buffer, BUFSIZE - 1)) > 0) {
        buffer[n] = '\0';
        printf("[THREAD %lu] client #%d -> %s",
               (unsigned long)pthread_self(), my_id, buffer);
        fflush(stdout);

        snprintf(reply, sizeof(reply), "[thread %lu] ECHO: %s",
                 (unsigned long)pthread_self(), buffer);
        if (write(connfd, reply, strlen(reply)) < 0) {
            perror("write");
            break;
        }
    }

    printf("[THREAD %lu] client #%d disconnected, thread exiting.\n",
           (unsigned long)pthread_self(), my_id);

    close(connfd);
    free(ca);
    return NULL;
}

int main(void)
{
    int listenfd, connfd;
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    pthread_t tid;
    client_arg_t *ca;
    int opt = 1;

    /* 1. socket / bind / listen */
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    if (listen(listenfd, BACKLOG) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    printf("[MT SERVER] Listening on port %d ...\n", PORT);

    for (;;) {
        clilen = sizeof(cliaddr);

        /* 2. Accept the next client */
        if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0) {
            perror("accept");
            continue;
        }

        /* 3. Pack the arguments and spawn a thread for this client */
        ca = malloc(sizeof(client_arg_t));
        if (ca == NULL) {
            perror("malloc");
            close(connfd);
            continue;
        }
        ca->connfd  = connfd;
        ca->cliaddr = cliaddr;

        if (pthread_create(&tid, NULL, handle_client, ca) != 0) {
            perror("pthread_create");
            close(connfd);
            free(ca);
            continue;
        }

        /* 4. Detach: the thread cleans itself up, main never joins it */
        pthread_detach(tid);
    }

    close(listenfd);
    return 0;
}
