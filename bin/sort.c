#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 10000
#define MAX_LEN 1024

int compare(const void *a, const void *b) { return strcmp(*(const char **)a, *(const char **)b); }

// TODO: Place on stack once we have support for growing stack
char *lines[MAX_LINES];

void sort(FILE *f) {
  char buffer[MAX_LEN];
  int count = 0;

  while (count < MAX_LINES && fgets(buffer, sizeof(buffer), f)) {
    lines[count] = strdup(buffer);
    if (!lines[count]) {
      fprintf(stderr, "Memory allocation error\n");
      exit(1);
    }
    count++;
  }

  qsort(lines, count, sizeof(char *), compare);

  for (int i = 0; i < count; i++) {
    printf("%s", lines[i]);
    free(lines[i]);
  }
}

int main(int argc, char *argv[]) {
  if (argc <= 1) {
    sort(stdin);
  }

  int ret = EXIT_SUCCESS;
  for (int i = 1; i < argc; i++) {
    FILE *f = fopen(argv[i], "r");
    if (!f) {
      perror("Error opening file");
      ret = EXIT_FAILURE;
    } else {
      sort(f);
      fclose(f);
    }
  }

  return ret;
}
