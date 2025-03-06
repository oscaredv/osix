#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char **argv) {
  if (argc > 1) {
    fprintf(stderr, "%s: No arguments expected\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  time_t t = time(NULL);
  printf("%s\n", ctime(&t));

  return EXIT_SUCCESS;
}
