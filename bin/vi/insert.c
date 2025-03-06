#include "vi.h"
#include <stdio.h>
#include <string.h>

void split_line() {
  for (int l = lines; l > row + 1; --l) {
    memcpy(buffer[l], buffer[l - 1], 256);
  }
  strncpy(buffer[row + 1], &buffer[row][col], 256);
  buffer[row][col] = 0;
  ++lines;
}

void insert_mode_input(char c) {
  if (c == 27) {
    mode = ModeCommand; // ESC
    move_cursor(row, col - 1);
    draw_line(row);
    draw_status();
  } else if (c == 127 && col > 0) { // Backspace
    printf("\b \b");
    buffer[row][col - 1] = 0;
    move_cursor(row, col - 1);
  } else if (c == '\n') {
    split_line();
    draw_screen();
    move_cursor(row + 1, 0);
  } else {
    printf("%c", c);
    memmove(&buffer[row][col + 1], &buffer[row][col], MAX_COLS - col - 1);
    buffer[row][col++] = c;
    draw_line(row);
    move_cursor(row, col);
  }
}
