/* ============================================================
 * Lab 6 (d) : SIGNAL-DRIVEN I/O model (SIGIO)
 * The kernel sends SIGIO when the socket becomes readable, so the
 * process can do other work instead of waiting.  A UDP socket is
 * used because SIGIO semantics are simplest for datagrams.
 * Compile : gcc sigdriven_io.c -o sigdriven_io
 * Test    : ./udp_client  (from Lab 2)  -> port 9090
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    9090
#define BUFSIZE 1024

static int   sockfd;
static volatile sig_atomic_t data_ready = 0;

/* SIGIO handler: only sets a flag (async-signal-safe) */
void sigio_handler(int signo)
{
    (void)signo;
    data_ready = 1;
}

int main(void)
{
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buf[BUFSIZE];
    ssize_t n;
    long work = 0;
    struct sigaction sa;
    int opt = 1;

    /* register the SIGIO handler */
    sa.sa_handler = sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGIO, &sa, NULL);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }

    /* enable signal-driven I/O on this socket */
    if (fcntl(sockfd, F_SETOWN, getpid()) < 0) {        /* who gets SIGIO  */
        perror("F_SETOWN"); exit(EXIT_FAILURE);
    }
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (fcntl(sockfd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) < 0) {
        perror("F_SETFL"); exit(EXIT_FAILURE);
    }

    printf("=== SIGNAL-DRIVEN I/O === UDP port %d\n", PORT);
    printf("Doing other work; the kernel will raise SIGIO when a "
           "datagram arrives.\n");

    for (;;) {
        if (!data_ready) {
            /* the process is free to do useful work while it waits */
            work++;
            if (work % 10 == 0) {
                printf("working ... (cycle %ld)\r", work);
                fflush(stdout);
            }
            usleep(200000);
            continue;
        }

        data_ready = 0;
        clilen = sizeof(cliaddr);
        while ((n = recvfrom(sockfd, buf, BUFSIZE - 1, 0,
                             (struct sockaddr *)&cliaddr, &clilen)) > 0) {
            buf[n] = '\0';
            printf("\nSIGIO received -> datagram from %s : %s",
                   inet_ntoa(cliaddr.sin_addr), buf);
            sendto(sockfd, buf, n, 0, (struct sockaddr *)&cliaddr, clilen);
            clilen = sizeof(cliaddr);
        }
    }

    close(sockfd);
    return 0;
}
