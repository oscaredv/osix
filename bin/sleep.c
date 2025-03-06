#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  int ret = 0;
  if (argc < 2) {
    fprintf(stderr, "%s: missing operand\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  for (int i = 1; i < argc; i++) {
    int s = atoi(argv[i]);
    if (s < 0) {
      ret = -1;
    }
    while (s > 0) {
      s = sleep(s);
    }
  }

  return ret;
}
