#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void count(FILE *f, int count_lines, int count_words, int count_bytes, const char *filename) {
  int lines = 0, words = 0, bytes = 0;
  int in_word = 0;
  int ch;

  while ((ch = fgetc(f)) != EOF) {
    bytes++;

    if (ch == '\n')
      lines++;

    if (isspace(ch)) {
      in_word = 0;
    } else if (!in_word) {
      words++;
      in_word = 1;
    }
  }

  if (count_lines || count_words || count_bytes) {
    if (count_lines)
      printf("%6d ", lines);
    if (count_words)
      printf("%6d ", words);
    if (count_bytes)
      printf("%6d ", bytes);
  } else {
    printf("%6d %6d %6d ", lines, words, bytes);
  }

  if (filename)
    printf("%s", filename);

  printf("\n");
}

int main(int argc, char *argv[]) {
  int count_lines = 0, count_words = 0, count_bytes = 0;
  int file_start = 1;

  // Parse command-line options
  if (argc > 1 && argv[1][0] == '-') {
    for (int i = 1; argv[1][i] != '\0'; i++) {
      switch (argv[1][i]) {
      case 'l':
        count_lines = 1;
        break;
      case 'w':
        count_words = 1;
        break;
      case 'c':
        count_bytes = 1;
        break;
      default:
        fprintf(stderr, "Usage: %s [-lwc] [file...]\n", argv[0]);
        return EXIT_FAILURE;
      }
    }
    file_start = 2;
  }

  if (argc == file_start) { // No files provided, read from stdin
    count(stdin, count_lines, count_words, count_bytes, NULL);
  } else {
    for (int i = file_start; i < argc; i++) {
      FILE *f = fopen(argv[i], "r");
      if (!f) {
        fprintf(stderr, "%s: Can't open file %s\n", argv[0], argv[i]);
        continue;
      }
      count(f, count_lines, count_words, count_bytes, argv[i]);
      fclose(f);
    }
  }

  return EXIT_SUCCESS;
}
