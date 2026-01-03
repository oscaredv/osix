#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_LINE 16

void print_data(FILE *f, int hex_mode) {
  uint8_t buffer[BYTES_PER_LINE];
  size_t offset = 0;
  ssize_t bytes_read;

  while ((bytes_read = fread(buffer, 1, BYTES_PER_LINE, f)) > 0) {
    printf("%07x ", offset);
    for (ssize_t x = 0; x < bytes_read; x++) {
      if (hex_mode)
        printf("%02x ", buffer[x]);
      else
        printf("%03o ", buffer[x]);
    }
    printf("\n");
    offset += bytes_read;
  }
}

int print_file(const char *filename, int hex_mode) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    fprintf(stderr, "od: Can't open file %s\n", filename);
    return EXIT_FAILURE;
  }

  print_data(f, hex_mode);
  fclose(f);
  return 0;
}

void usage() {
  printf("usage: od [-x] [FILE]\n");
  printf("  -x    Display output in hexadecimal (default is octal)\n");
  printf("  -h    Show this help message\n");
  exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
  int hex_mode = 0;
  int file_count = 0;
  int errors = 0;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-x")) {
      hex_mode = 1;
    } else if (strcmp(argv[1], "-h") == 0) {
      usage();
    } else if (strcmp(argv[1], "-") == 0) {
      file_count++;
      print_data(stdin, hex_mode);
    } else {
      file_count++;
      if (print_file(argv[i], hex_mode) != 0) {
        errors++;
      }
    }
  }

  if (file_count == 0)
    print_data(stdin, hex_mode);

  if (errors > 0)
    return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
