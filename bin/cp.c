#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "Usage: cp <source> <dest>\n");
    return EXIT_FAILURE;
  }

  int src = open(argv[1], O_RDONLY);
  if (src < 0) {
    perror(argv[1]);
    return EXIT_FAILURE;
  }

  int dst = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (dst < 0) {
    perror(argv[2]);
    close(src);
    return EXIT_FAILURE;
  }

  char buf[4096];
  int n;
  while ((n = read(src, buf, sizeof(buf))) > 0) {
    if (write(dst, buf, n) != n) {
      perror(argv[2]);
      close(src);
      close(dst);
      return EXIT_FAILURE;
    }
  }

  close(src);
  close(dst);
  return EXIT_SUCCESS;
}
