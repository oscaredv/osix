#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE 1024
#define DEFAULT_MARGIN 7

const char *section(char s) {
  switch (s) {
  case '1':
    return "General Commands Manual";
  case '2':
    return "System Calls Manual";
  case '3':
    return "Library Functions Manual";
  case '4':
    return "Kernel Interfaces Manual";
  case '5':
    return "File Formats Manual";
  case '6':
    return "Games Manual";
  case '7':
    return "Miscellaneous Manual";
  case '8':
    return "System Manager Manual";
  case '9':
    return "Kernel Developer Manual";
  }
  return "";
}

int offset = 0;

void roff_print(const char *s) {
  while (*s != 0) {
    if (*s == '\\') {
      s++;
    } else {
      fwrite(s++, 1, 1, stdout);
    }
  }
}

int margin = DEFAULT_MARGIN;

void roff_word(char *word) {
  int wlen = strlen(word);
  if (offset + wlen >= 80) {
    printf("\n");
    offset = 0;
  }

  if (offset == 0) {
    while (offset < margin) {
      printf(" ");
      ++offset;
    }

    if (word[0] == ' ')
      return;
  }

  roff_print(word);
  offset += wlen;
}

void roff(char **words) {
  int w = 0;
  while (words[w] != NULL) {
    if (offset > 8)
      roff_word(" ");
    roff_word(words[w++]);
  }
}

int tp = 0;
int no_fill = 0;
void macros(char **words) {
  if (words[0] == NULL)
    return;

  if (strcmp(words[0], ".TH") == 0) {
    int title_len = strlen(words[1]) + strlen(words[2]) + 2;
    int col = title_len;
    printf("\r\e[1m%s(%s)\e[0m", words[1], words[2]);
    const char *section_name = section(words[2][0]);
    while (col < 28) {
      printf(" ");
      col++;
    }
    col += strlen(section_name);
    printf("%s", section_name);
    while (col < (80 - title_len - 2)) {
      printf(" ");
      col++;
    }
    printf("\e[1m%s(%s)\e[0m", words[1], words[2]);
    margin = DEFAULT_MARGIN;
    offset = 0;
  } else if (strcmp(words[0], ".SH") == 0) {
    printf("\n\n\e[1m%s\e[0m\n", words[1]);
    margin = DEFAULT_MARGIN;
    offset = 0;
  } else if (strcmp(words[0], ".SS") == 0) {
    printf("\n\n   \e[1m%s\e[0m\n", words[1]);
    margin = DEFAULT_MARGIN;
    offset = 0;
  } else if (strcmp(words[0], ".TP") == 0) {
    if (offset)
      printf("\n\n");
    printf("       ");
    tp = 1;
    offset = 7;
  } else if (strcmp(words[0], ".PP") == 0) {
    if (offset > 0) {
      printf("\n");
      offset = 0;
    }
    printf("\n");
    margin = DEFAULT_MARGIN;
  } else if (strcmp(words[0], ".IP") == 0) {
    if (offset > 0) {
      printf("\n");
      offset = 0;
    }
    printf("\n");
    margin = 14;
  } else if (strcmp(words[0], ".I") == 0) {
    printf("\e[3m");
    roff(&words[1]);
    printf("\e[0m");
  } else if (strcmp(words[0], ".B") == 0) {
    printf("\e[1m");
    roff(&words[1]);
    printf("\e[0m");
  } else if (strcmp(words[0], ".IR") == 0) {
    int w = 1;
    while (words[w] != NULL) {
      if (offset > 8 && !ispunct(words[w][0]))
        roff_word(" ");
      if (w & 1)
        printf("\e[1m");
      roff_word(words[w]);
      if (w & 1)
        printf("\e[0m");
      ++w;
    }
  } else if (strcmp(words[0], ".nf") == 0) {
    no_fill = 1;
  } else {
    roff(words);
  }

  if (tp == 1) {
    tp++;
  } else if (tp == 2) {
    margin = 12;
    while (offset < margin) {
      printf(" ");
      ++offset;
    }
    tp = 0;
  }
}

void process_file(FILE *fp) {
  char line[MAX_LINE];
  while (fgets(line, sizeof(line), fp)) {
    if (no_fill) {
      if (strncmp(line, ".fi", 3) == 0) {
        no_fill = 0;
        continue;
      }
      printf("        %s", line);
      continue;
    }

    char *words[256];
    memset(words, 0, sizeof(words));
    int c = 0;
    int w = 0;
    while (line[c] != 0) {
      while (isspace(line[c]))
        c++;

      if (line[c] == 0)
        break;

      if (line[c] == '"') {
        c++;
        words[w] = &line[c];

        while (line[c] != 0 && line[c] != '\n' && line[c] != '"')
          c++;

        line[c++] = 0;
      } else {
        words[w] = &line[c];

        while (line[c] != 0 && !isspace(line[c]))
          c++;

        line[c++] = 0;
      }
      w++;
    }

    macros(words);
  }
  printf("\n");
}

int main(int argc, char *argv[]) {
  FILE *fp = stdin;
  if (argc > 1) {
    fp = fopen(argv[1], "r");
    if (!fp) {
      perror("Error opening file");
      return EXIT_FAILURE;
    }
  }
  process_file(fp);
  if (fp != stdin) {
    fclose(fp);
  }
  return 0;
}
