#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define TERM_ROWS 22

int lines = 0;

void disable_raw_mode() {
  struct termios cooked;
  tcgetattr(STDIN_FILENO, &cooked);
  cooked.c_lflag |= ECHO | ICANON;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
}

void enable_raw_mode() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

int waitkey() {
  enable_raw_mode();
  int ch = getchar();
  disable_raw_mode();
  return ch;
}

void more(FILE *f) {
  char line[1024];
  while (fgets(line, sizeof(line), f)) {
    fputs(line, stdout);
    if (lines > 0)
      --lines;

    if (lines == 0) {
      printf("--More--");
      fflush(stdout);
      int c = waitkey();
      printf("\r          \r");

      if (c == 'q' || c == 27)
        break;
      if (c == '\r' || c == '\n')
        lines = 1;
      else
        lines = TERM_ROWS;
    }
  }
}

void usage() {
  fprintf(stderr, "usage: more [file ...]\n");
  exit(EXIT_FAILURE);
}

void signal_handler(int signo) { exit(signo); }

int main(int argc, char *argv[]) {
  atexit(disable_raw_mode);
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  lines = TERM_ROWS;

  int arg = 1;
  int option = 1;
  while (arg < argc && argv[arg][0] == '-' && argv[arg][1] != 0) {
    switch (argv[arg][option]) {
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
    more(stdin);
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
      more(f);
      if (f != stdin)
        fclose(f);
    }

    ++arg;
  }
  return ret;
}
