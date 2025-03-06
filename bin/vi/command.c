#include "vi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void replace_char(int count) {
  char c;
  if (read(STDIN_FILENO, &c, 1) == 1) {
    if (col + count <= (int)strlen(buffer[row])) {
      for (int i = col; i < col + count; i++) {
        buffer[row][i] = c;
      }
      draw_line(row);
    }
  }
}

void run_command(int count, const char *cmd) {
  int real_count = count;
  if (count < 1)
    count = 1;

  if (strcmp(cmd, "i") == 0) {
    mode = ModeInsert;
  } else if (strcmp(cmd, "a") == 0) {
    mode = ModeInsert;
    move_cursor(row, col + 1);
  } else if (strcmp(cmd, "r") == 0) {
    mode = ModeReplaceChar;
    draw_status();
    replace_char(count);
    mode = ModeCommand;
  } else if (strcmp(cmd, "h") == 0 || strcmp(cmd, "\e[D") == 0) { // left
    move_cursor(row, col - count);
  } else if (strcmp(cmd, "l") == 0 || strcmp(cmd, "\e[C") == 0) { // right
    move_cursor(row, col + count);
  } else if (strcmp(cmd, "k") == 0 || strcmp(cmd, "\e[A") == 0) { // up
    move_cursor(row - count, col);
  } else if (strcmp(cmd, "j") == 0 || strcmp(cmd, "\e[B") == 0) { // down
    move_cursor(row + count, col);
  } else if (strcmp(cmd, "\e[5~") == 0) { // page up
    move_cursor(row - ((ROWS - 1) * count) + 1, col);
  } else if (strcmp(cmd, "\e[6~") == 0) { // page down
    move_cursor(row + ((ROWS - 1) * count) - 1, col);
  } else if (strcmp(cmd, "\e[H") == 0 || strcmp(cmd, "0") == 0) { // Home
    move_cursor(row, 0);
  } else if (strcmp(cmd, "\e[F") == 0 || strcmp(cmd, "$") == 0) { // End
    int linelen = strlen(buffer[row + row_offset]);
    if (linelen > 0)
      linelen--;
    move_cursor(row, linelen);
  } else if (strcmp(cmd, "x") == 0) {
    if (col + count <= (int)strlen(buffer[row])) {
      memmove(&buffer[row][col], &buffer[row][col + count], strlen(&buffer[row][col]));
      draw_line(row);
      move_cursor(row, col);
    }
  } else if (strcmp(cmd, "ZZ") == 0) {
    clear_screen();
    save();
    quit();
  } else if (strcmp(cmd, "dd") == 0) {
    delete_lines(row, count);
    move_cursor(row, col);
  } else if (strcmp(cmd, "dw") == 0) {
    delete_word(row, col, count, 1);
    move_cursor(row, col);
  } else if (strcmp(cmd, "cw") == 0) {
    delete_word(row, col, count, 0);
    mode = ModeInsert;
    move_cursor(row, col);
  } else if (strcmp(cmd, "G") == 0) {
    if (real_count != 0)
      move_cursor(count - 1, col);
    else
      move_cursor(lines - 1, col);
  } else if (strcmp(cmd, ":") == 0) {
    printf("\e[%d;%df", ROWS, 0);
    printf("\e[2K\r:");
    fflush(stdout);
    mode = ModeCommandLine;
  }
}

void command_mode_input(char c) {
  static int cmdlen = 0;
  static char cmd[8];
  static int count = 0;

  if (cmdlen == 0 && (c >= '1' || (c >= '0' && count > 0)) && c <= '9') {
    count *= 10;
    count += c - '0';
    snprintf(message, 32, "count=%d cmd=%s", count, cmd);
  } else {
    cmd[cmdlen++] = c;
    char E = cmd[0];
    if (cmd[0] == '\e')
      cmd[0] = 'E';
    snprintf(message, 32, "count=%d cmd='%s'", count, cmd);
    cmd[0] = E;

    // Incomplete command? Wait for more input
    if (strcmp(cmd, "d") == 0)
      return;
    if (strcmp(cmd, "c") == 0)
      return;
    if (strcmp(cmd, "Z") == 0)
      return;
    if (strcmp(cmd, "\e") == 0 || strcmp(cmd, "\e[") == 0 || strcmp(cmd, "\e[5") == 0 || strcmp(cmd, "\e[6") == 0)
      return;

    run_command(count, cmd);
    memset(cmd, 0, sizeof(cmd));
    cmdlen = 0;
    count = 0;
  }
}
