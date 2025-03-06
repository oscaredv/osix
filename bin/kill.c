#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
  if (argc < 1) {
    fprintf(stderr, "%s: No pid argument.", argv[0]);
    return -1;
  }

  int sig = SIGTERM;

  int a = 1;
  if (argc >= a && *argv[a] == '-') {
    if (!strcmp("-SIGHUP", argv[a]))
      sig = SIGHUP;
    else if (!strcmp("-SIGINT", argv[a]))
      sig = SIGINT;
    else if (!strcmp("-SIGKILL", argv[a]))
      sig = SIGKILL;
    else if (!strcmp("-SIGSEGV", argv[a]))
      sig = SIGSEGV;
    else if (!strcmp("-SIGTERM", argv[a]))
      sig = SIGTERM;
    else
      sig = atoi(&argv[a][1]);

    a++;
  }

  int exit_code = EXIT_SUCCESS;
  for (; a < argc; a++) {
    pid_t pid = atoi(argv[a]);
    if (kill(pid, sig) == -1) {
      fprintf(stderr, "kill: Failed to kill pid %d\n", pid);
      exit_code = EXIT_FAILURE;
    }
  }
  return exit_code;
}
