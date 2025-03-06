#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void usage(int ret) {
  fprintf(stderr, "Usage: rm [file...]\n");
  exit(ret);
}

int main(int argc, char **argv) {
  int ret = EXIT_SUCCESS;
  if (argc < 2) {
    usage(EXIT_FAILURE);
  }

  for (int i = 1; i < argc; i++) {
    if (unlink(argv[i]) != 0) {
      ret = EXIT_FAILURE;
      fprintf(stderr, "rm: cannot remove %s\n", argv[i]);
    }
  }

  return ret;
}
