#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

void cat(int fd) {
  char buf[256];
  int len = read(fd, buf, 256);
  while (len != 0) {
    buf[len] = 0;
    write(STDOUT_FILENO, buf, len);
    len = read(fd, buf, 255);
  }
}

int main(int argc, char **argv) {
  int ret = EXIT_SUCCESS;
  if (argc == 1) {
    cat(STDIN_FILENO);
  }

  for (int i = 1; i < argc; i++) {
    if (strcmp("-", argv[i]) == 0) {
      cat(STDERR_FILENO);
    } else {
      int fd = open(argv[i], O_RDONLY);
      if (fd == -1) {
        fprintf(stderr, "%s: %s: No such file or directory\n", argv[0], argv[i]);
        ret = EXIT_FAILURE;
      } else {
        struct stat st;
        fstat(fd, &st);
        if ((st.st_mode & S_IFMT) == S_IFDIR) {
          fprintf(stderr, "%s: %s: Is a directory\n", argv[0], argv[i]);
          ret = EXIT_FAILURE;
        } else {
          cat(fd);
        }
        close(fd);
      }
    }
  }

  return ret;
}
