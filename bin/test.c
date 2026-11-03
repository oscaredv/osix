#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int is_file(const char *path) {
  struct stat st;

  if (stat(path, &st) != 0)
    return 0;

  return S_ISREG(st.st_mode);
}

static int is_dir(const char *path) {
  struct stat st;

  if (stat(path, &st) != 0)
    return 0;

  return S_ISDIR(st.st_mode);
}

static int parse_int(const char *s, long *value) {
  char *end;

  *value = strtol(s, &end, 10);

  return *s != '\0' && *end == '\0';
}

static int numeric_compare(const char *a, const char *op, const char *b) {
  long x;
  long y;

  if (!parse_int(a, &x) || !parse_int(b, &y)) {
    fprintf(stderr, "test: integer expression expected\n");
    return 2;
  }

  if (strcmp(op, "-eq") == 0)
    return x == y ? 0 : 1;

  if (strcmp(op, "-ne") == 0)
    return x != y ? 0 : 1;

  if (strcmp(op, "-gt") == 0)
    return x > y ? 0 : 1;

  if (strcmp(op, "-lt") == 0)
    return x < y ? 0 : 1;

  if (strcmp(op, "-ge") == 0)
    return x >= y ? 0 : 1;

  if (strcmp(op, "-le") == 0)
    return x <= y ? 0 : 1;

  return 2;
}

int main(int argc, char *argv[]) {
  // [
  if (strcmp(argv[0], "[") == 0 || (strrchr(argv[0], '/') && strcmp(strrchr(argv[0], '/') + 1, "[") == 0)) {
    if (argc < 2 || strcmp(argv[argc - 1], "]") != 0) {
      fprintf(stderr, "[: missing ]\n");
      return 2;
    }

    argc--;
    argv[argc] = NULL;
  }

  // test -> false
  if (argc == 1)
    return 1;

  // test STRING
  if (argc == 2) {
    return argv[1][0] == '\0';
  }

  // -f
  if (argc == 3 && strcmp(argv[1], "-f") == 0) {
    return !is_file(argv[2]);
  }

  // -d
  if (argc == 3 && strcmp(argv[1], "-d") == 0) {
    return !is_dir(argv[2]);
  }

  // STR1 = STR2
  if (argc == 4 && strcmp(argv[2], "=") == 0) {
    return strcmp(argv[1], argv[3]) == 0 ? 0 : 1;
  }

  // STR1 != STR2
  if (argc == 4 && strcmp(argv[2], "!=") == 0) {
    return strcmp(argv[1], argv[3]) != 0 ? 0 : 1;
  }

  // Numeric operators
  if (argc == 4) {
    return numeric_compare(argv[1], argv[2], argv[3]);
  }

  fprintf(stderr, "test: unsupported expression\n");
  return 2;
}