#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_LINE 16

void print_data(FILE *f, int hex_mode) {
  uint8_t buffer[BYTES_PER_LINE];
  size_t offset = 0;
  size_t bytes_read;

  while ((bytes_read = fread(buffer, 1, BYTES_PER_LINE, f)) > 0) {
    printf("%07x ", offset);
    for (size_t x = 0; x < bytes_read; x++) {
      if (hex_mode)
        printf("%02x ", buffer[x]);
      else
        printf("%03o ", buffer[x]);
    }
    printf("\n");
    offset += bytes_read;
  }
}

int main(int argc, char *argv[]) {
  int hex_mode = 0;
  char *filename = NULL;

  if (argc > 1) {
    if (strcmp(argv[1], "-x") == 0) {
      hex_mode = 1;
      if (argc > 2) {
        filename = argv[2];
      }
    } else {
      filename = argv[1];
    }
  }

  FILE *f = stdin;
  if (filename) {
    f = fopen(filename, "rb");
    if (!f) {
      fprintf(stderr, "%s: Can't open file %s\n", argv[0], filename);
      return EXIT_FAILURE;
    }
  }

  print_data(f, hex_mode);

  if (f != stdin) {
    fclose(f);
  }

  return EXIT_SUCCESS;
}
