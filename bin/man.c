#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

void usage() {
  fprintf(stderr, "usage: man [-a] [section] MANPAGE...\n");
  exit(EXIT_SUCCESS);
}

void man(const char *page) {
  int child = fork();
  if (child == 0) {
    execl("/bin/roff", "roff", page, NULL);
  } else if (child > 0) {
    int exit_code = 0;
    while (wait(&exit_code) == -1)
      ;
  }
}

int section = -1;
int all = 0;

int main(int argc, char **argv) {
  int a = 1;
  while (a < argc && (argv[a][0] == '-' || (argv[a][0] >= '0' && argv[a][0] <= '9' && argv[a][1] == 0))) {
    if (argv[a][0] == '-') {
      switch (argv[a][1]) {
      case 'a':
        all = 1;
        break;
      case 'h':
        usage();
        break;
      default:
        fprintf(stderr, "Bad option: %s\n", argv[a]);
        usage();
        break;
      }
    } else {
      section = argv[a][0] - '0';
    }
    ++a;
  }

  int total = 0;
  while (a < argc) {
    char page[PATH_MAX];
    int count = 0;

    int sect_begin = 0;
    int sect_end = 10;
    if (section != -1) {
      sect_begin = section;
      sect_end = section + 1;
    }
    for (int s = sect_begin; s < sect_end; s++) {
      snprintf(page, PATH_MAX, "/usr/man/man%d/%s.%d", s, argv[a], s);
      struct stat st;
      if (stat(page, &st) == 0) {
        count++;
        if (total > 0) {
          printf("--Man-- next: %s(%d) [ view (return) | skip (ctrl-d) | quit (ctrl-c) ]\n", argv[a], s);
          char input;
          if (read(fileno(stdin), &input, 1) == 0)
            continue;
        }

        total++;
        man(page);
        if (!all)
          break;
      }
    }

    if (count == 0) {
      total++;
      if (section != -1)
        fprintf(stderr, "No manual page for %s in section %d\n", argv[a], section);
      else
        fprintf(stderr, "No manual page for %s\n", argv[a]);
    }
    ++a;
  }

  if (total == 0) {
    fprintf(stderr, "Whan manual page? Try: man man\n");
    exit(EXIT_FAILURE);
  }
  return EXIT_SUCCESS;
}
