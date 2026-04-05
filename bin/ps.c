#include <stdio.h>
#include <stdlib.h>
#include <sys/procinfo.h>
#include <sys/types.h>

const char *state_to_string(int state) {
  switch (state) {
  case STATE_NULL:
    break;
  case STATE_IDLE:
    return "IDLE  ";
  case STATE_RUNNING:
    return "RUN   ";
  case STATE_SLEEP:
    return "SLEEP ";
  case STATE_ZOMBIE:
    return "ZOMBIE";
  }
  return "???   ";
}

int main() {
  struct procinfo *buf;
  size_t len = 0;

  if (getprocs(NULL, &len) < 0) {
    fprintf(stderr, "ps: failed to get process count\n");
    return EXIT_FAILURE;
  }

  buf = malloc(len);
  if (!buf) {
    fprintf(stderr, "ps: failed to allocate memory\n");
    return EXIT_FAILURE;
  }

  if (getprocs(buf, &len) < 0) {
    fprintf(stderr, "ps: failed to get process info\n");
    free(buf);
    return EXIT_FAILURE;
  }

  printf("  PID  PPID  STATE  COMMAND\n");
  size_t count = len / sizeof(struct procinfo);
  for (size_t i = 0; i < count; i++) {
    printf("%5d %5d  %s %s\n", buf[i].pid, buf[i].ppid, state_to_string(buf[i].state), buf[i].name);
  }

  free(buf);
  return EXIT_SUCCESS;
}