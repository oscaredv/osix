#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN 1024

void uniq(FILE *f, int print_unique) {
  char previous[MAX_LEN] = "";
  char current[MAX_LEN];
  int count = 0;

  while (fgets(current, sizeof(current), f)) {
    if (strcmp(current, previous) == 0) {
      count++;
    } else {
      if (!print_unique || count == 1) {
        if (previous[0] != 0) {
          printf("%s", previous);
        }
      }
      strncpy(previous, current, MAX_LEN);
      count = 1;
    }
  }

  if (!print_unique || count == 1) {
    printf("%s", previous);
  }
}

void usage() {
  fprintf(stderr, "usage: uniq [-u] [file ...]\n");
  exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
  int print_unique = 0;

  int arg = 1;
  int option = 1;
  while (arg < argc && argv[arg][0] == '-' && argv[arg][1] != 0) {
    switch (argv[arg][option]) {
    case 'u':
      print_unique = 1;
      break;
    case 'h':
      usage();
      break;
    default:
      usage();
      break;
    }

    ++option;
    if (argv[arg][option] == 0) {
      option = 1;
      ++arg;
    }
  }

  if (arg == argc) {
    uniq(stdin, print_unique);
  }

  int ret = EXIT_SUCCESS;
  while (arg < argc) {
    FILE *f = stdin;
    if (strcmp(argv[arg], "-") != 0) {
      f = fopen(argv[arg], "r");
    }
    if (!f) {
      perror(argv[arg]);
      ret = EXIT_FAILURE;
    } else {
      uniq(f, print_unique);
      if (f != stdin)
        fclose(f);
    }

    ++arg;
  }
  return ret;
}
