/* ============================================================
 * Lab 8 : Daemonizing a Process
 * Compile : gcc daemonize.c -o daemonize
 * Run     : ./daemonize
 * Verify  : ps -ef | grep daemonize      (TTY column shows '?')
 *           tail -f /tmp/mydaemon.log
 *           kill <pid>                   to stop it
 * ============================================================ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define LOGFILE "/tmp/mydaemon.log"

static volatile sig_atomic_t keep_running = 1;

void term_handler(int signo) { (void)signo; keep_running = 0; }

int main(void)
{
    pid_t pid;
    FILE *log;
    int i;

    /* 1. First fork -> the parent exits, so the child is not a group leader */
    if ((pid = fork()) < 0) { perror("fork"); exit(EXIT_FAILURE); }
    if (pid > 0) {
        printf("Parent (PID %d) exiting; daemon is starting.\n", getpid());
        exit(EXIT_SUCCESS);
    }

    /* 2. New session -> detach from the controlling terminal */
    if (setsid() < 0) { perror("setsid"); exit(EXIT_FAILURE); }

    /* 3. Ignore terminal-related signals; handle SIGTERM for a clean stop */
    signal(SIGHUP,  SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    signal(SIGTERM, term_handler);

    /* 4. Second fork -> the daemon can never re-acquire a terminal */
    if ((pid = fork()) < 0) { perror("fork"); exit(EXIT_FAILURE); }
    if (pid > 0) exit(EXIT_SUCCESS);

    /* 5. Clear the file mode creation mask */
    umask(0);

    /* 6. Change working directory to / so no filesystem stays locked */
    if (chdir("/") < 0) { exit(EXIT_FAILURE); }

    /* 7. Close all inherited descriptors and redirect std fds to /dev/null */
    for (i = sysconf(_SC_OPEN_MAX) - 1; i >= 0; i--)
        close(i);
    open("/dev/null", O_RDONLY);          /* stdin  -> fd 0 */
    open("/dev/null", O_WRONLY);          /* stdout -> fd 1 */
    open("/dev/null", O_WRONLY);          /* stderr -> fd 2 */

    /* 8. The actual daemon task: append a heartbeat to the log every 5 s */
    while (keep_running) {
        if ((log = fopen(LOGFILE, "a")) != NULL) {
            time_t now = time(NULL);
            char ts[32];
            strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", localtime(&now));
            fprintf(log, "[%s] Daemon (PID %d) is alive.\n", ts, getpid());
            fclose(log);
        }
        sleep(5);
    }

    if ((log = fopen(LOGFILE, "a")) != NULL) {
        fprintf(log, "Daemon (PID %d) received SIGTERM, exiting.\n", getpid());
        fclose(log);
    }
    return 0;
}
