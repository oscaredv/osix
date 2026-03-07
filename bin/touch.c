#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

int touch(const char *filename) {
  struct stat st;

  if (stat(filename, &st) == -1) {
    if (errno == ENOENT) {
      // File does not exist, create it
      int fd = creat(filename, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
      if (fd == -1) {
        perror(filename);
        return EXIT_FAILURE;
      }
      close(fd);
    } else {
      perror(filename);
      return EXIT_FAILURE;
    }
  } else if (utime(filename, NULL) == -1) {
    perror(filename);
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "usage: touch FILE ...\n");
    return EXIT_FAILURE;
  }

  int ret = EXIT_SUCCESS;
  for (int f = 1; f < argc; f++) {
    if (touch(argv[f]) == EXIT_FAILURE) {
      ret = EXIT_FAILURE;
    }
  }

  return ret;
}