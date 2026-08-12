/* =====================================================================
 * Lab 6 (d) : SIGNAL-DRIVEN I/O model  (SIGIO)
 * The kernel sends SIGIO when the socket becomes readable; the process
 * is free to do other work until the handler fires.  A UDP socket is
 * used because SIGIO semantics are simplest and most reliable there.
 * Compile : gcc 4_signal_driven_io.c -o signal_driven
 * Run     : ./signal_driven     then send data with:
 *           echo "hello" | nc -u 127.0.0.1 9090
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT     9090
#define BUFSIZE  1024

static int sockfd;
static volatile sig_atomic_t data_ready = 0;   /* set by the handler */

/* The handler must stay minimal: it only raises a flag.  The real
   reading is done by the main loop (async-signal-safety).            */
void sigio_handler(int signo)
{
    (void)signo;
    data_ready = 1;
}

int main(void)
{
    struct sockaddr_in servaddr, cliaddr;
    socklen_t clilen;
    char buffer[BUFSIZE];
    ssize_t n;
    int flags, opt = 1;
    struct sigaction sa;
    long idle = 0;

    /* 1. Create and bind a UDP socket */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket"); exit(EXIT_FAILURE);
    }
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family      = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port        = htons(PORT);

    if (bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("bind"); exit(EXIT_FAILURE);
    }

    /* 2. Install the SIGIO handler */
    sa.sa_handler = sigio_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGIO, &sa, NULL) < 0) {
        perror("sigaction"); exit(EXIT_FAILURE);
    }

    /* 3. Tell the kernel WHO should receive SIGIO for this socket */
    if (fcntl(sockfd, F_SETOWN, getpid()) < 0) {
        perror("fcntl F_SETOWN"); exit(EXIT_FAILURE);
    }

    /* 4. Enable asynchronous (signal-driven) + non-blocking I/O */
    flags = fcntl(sockfd, F_GETFL, 0);
    if (fcntl(sockfd, F_SETFL, flags | O_ASYNC | O_NONBLOCK) < 0) {
        perror("fcntl F_SETFL"); exit(EXIT_FAILURE);
    }

    printf("=== SIGNAL-DRIVEN I/O MODEL (UDP port %d) ===\n", PORT);
    printf("Doing other work; the kernel will interrupt us with SIGIO.\n");
    printf("Send data with:  echo hello | nc -u 127.0.0.1 %d\n\n", PORT);

    for (;;) {
        if (!data_ready) {
            /* ---- the process is free to do anything else ---- */
            printf("working... (idle tick %ld)\r", ++idle);
            fflush(stdout);
            sleep(1);
            continue;
        }

        /* ---- SIGIO fired : drain every pending datagram ---- */
        data_ready = 0;
        printf("\n>>> SIGIO received! Reading the socket now.\n");

        for (;;) {
            clilen = sizeof(cliaddr);
            n = recvfrom(sockfd, buffer, BUFSIZE - 1, 0,
                         (struct sockaddr *)&cliaddr, &clilen);
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                perror("recvfrom");
                break;
            }
            buffer[n] = '\0';
            printf("    From %s:%d -> %s\n", inet_ntoa(cliaddr.sin_addr),
                   ntohs(cliaddr.sin_port), buffer);

            sendto(sockfd, "ECHO (SIGIO)\n", 13, 0,
                   (struct sockaddr *)&cliaddr, clilen);
        }
        printf(">>> Back to normal work.\n\n");
    }

    close(sockfd);
    return 0;
}
