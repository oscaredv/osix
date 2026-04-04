#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: mkdir <dir>\n");
    return EXIT_FAILURE;
  }

  for (int i = 1; i < argc; i++) {
    if (mkdir(argv[i], 0755) == -1) {
      perror(argv[i]);
    }
  }

  return EXIT_SUCCESS;
}
