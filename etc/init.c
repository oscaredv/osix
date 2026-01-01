#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define TTY_NAME_LEN 16
#define MAX_TTYS 16

int tty_count;

struct tty {
  short pid;
  char name[TTY_NAME_LEN];
} ttys[MAX_TTYS];

void single() {
  printf("Single user mode. Type \"exit\" or CTRL-d to continue boot\n");
  short child = fork();
  if (child == -1) {
    fprintf(stderr, "fork failed!\n");
    exit(-1);
  } else if (child == 0) {
    // Start root shell
    exit(execl("/bin/sh", "/bin/sh", "-", NULL));
  } else {
    int status;
    int pid = 0;
    do {
      pid = wait(&status);
    } while (pid != child);

    if (status != 0) {
      printf("sh exit status=%d\n", status);
    }
  }
}

void readttys() {
  FILE *etc_ttys = fopen("/etc/ttys", "r");
  tty_count = 0;
  if (etc_ttys) {
    char line[256];
    while (tty_count < MAX_TTYS && fgets(line, sizeof(line), etc_ttys)) {
      if (*line != 0 && *line != '#') {
        int len = strlen(line);
        line[len - 1] = 0;

        ttys[tty_count].pid = -1;
        snprintf(ttys[tty_count].name, TTY_NAME_LEN, "%s", line);
        tty_count++;
      }
    }
    fclose(etc_ttys);
  }
}

void multi() {
  printf("Entering multi-user\n");

  readttys();

  while (1) {
    // Wait for child processes to exit. The PID of the child process is
    // returned. If init has no child processes, as when no getty processes
    // has been started, -1 is returned.
    short pid = wait(NULL);

    for (int t = 0; t < tty_count; t++) {
      if (ttys[t].pid == pid || pid == -1) {
        // Fork and exec getty
        short child = fork();
        if (child > 0) {
          // We're the parent process, remember the child pid
          ttys[t].pid = child;
        } else if (child == 0) {
          // We're the child process, exec getty
          execl("/etc/getty", "/etc/getty", ttys[t].name, NULL);
        }
      }
    }
  }

  printf("Shutting down...\n");
}

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  // Open stdin, stdout & stderr
  int fd = open("/dev/console", O_RDWR);
  dup(fd);
  dup(fd);

  setenv("PATH", "/bin", 1);

  while (1) {
    single();
    multi();
  }

  return 0;
}
