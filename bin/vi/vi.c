#include "vi.h"
#include <ctype.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <termios.h>
#include <unistd.h>

char filename[PATH_MAX];

char message[32];

int row_offset = 0;

char buffer[MAX_ROWS][MAX_COLS];
int row = 0, col = 0, want_col = 0;
int lines = 0;

enum Mode mode = ModeEx;

void disable_raw_mode() {
  struct termios cooked;
  tcgetattr(STDIN_FILENO, &cooked);
  cooked.c_lflag |= ECHO | ICANON;
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
}

void enable_raw_mode() {
  atexit(disable_raw_mode);
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void save_as(const char *pathname) {
  snprintf(filename, PATH_MAX, "%s", pathname);
  save();
}

void save() {
  if (*filename == 0) {
    fprintf(stderr, "*** NO FILENAME!! ***\n");
    return;
  }
  FILE *out = fopen(filename, "w");
  if (out == NULL) {
    fprintf(stderr, "Unable to open %s for writing!\n", filename);
    return;
  }

  for (int line = 0; line < lines; line++) {
    fprintf(out, "%s\n", buffer[line]);
  }
  fclose(out);
  printf("File '%s' saved\n", filename);
}

void clear_screen() {
  printf("\e[2J");
  printf("\e[H");
  fflush(stdout);
}

void quit() { exit(EXIT_SUCCESS); }

void draw_screen() {
  clear_screen();
  for (int i = row_offset; i - row_offset < ROWS - 1; i++) {
    draw_line(i);
  }
  fflush(stdout);
}

void draw_line(int row) {
  printf("\e[%d;%df", row + 1 - row_offset, 1);
  printf("\e[2K");
  if (row < lines) {
    printf("%s\n", buffer[row]);
  } else {
    printf("~\n");
  }
}

void delete_lines(int row, int count) {
  if (count == 0 || (row + count) > lines)
    return;

  for (int r = row; r < lines; r++) {
    int src = r + count;
    if (src < lines) {
      memcpy(buffer[r], buffer[src], MAX_COLS);
    } else {
      memset(buffer[r], 0, MAX_COLS);
    }
  }

  lines -= count;
  if (lines < 1)
    lines = 1;

  for (int r = row; r < ROWS - 3; r++) {
    draw_line(r);
  }
}

void delete_word(int row, int col, int count, char remove_whitespace_after) {
  (void)count;
  char *p = &buffer[row][col + 1];
  while (*p != 0 && !isspace(*p) && !ispunct(*p))
    ++p;
  if (remove_whitespace_after) {
    while (isspace(*p))
      ++p;
  }

  int e = p - (char *)&buffer[row];
  memmove(&buffer[row][col], p, MAX_COLS - e);
  draw_line(row);
}

void update_cursor() {
  printf("\e[%d;%df", row + 1 - row_offset, col + 1);
  fflush(stdout);
}

void move_cursor(int r, int c) {
  if (c < 0)
    c = 0;
  if (r < 0)
    r = 0;

  if (c == col && row != r && want_col > c) {
    col = want_col;
  } else {
    col = c;
  }

  row = r;
  if (row >= lines) {
    row = lines - 1;
  }

  if (row >= row_offset + ROWS - 1) {
    row_offset += row - (row_offset + ROWS - 2);
    // printf("\e[%d;%df\n", ROWS, 0);
    draw_screen();
  } else if (row < row_offset) {
    row_offset -= row_offset - row;
    draw_screen();
  }

  int linelen = strlen(buffer[row]);
  int last_col = 0;
  if (linelen > 0) {
    if (mode == ModeCommand) {
      last_col = linelen - 1;
    } else {
      last_col = linelen;
    }
  }

  if (col > last_col) {
    want_col = col;
    col = last_col;
  }

  update_cursor();
}

void draw_status() {
  printf("\e[%d;%df", ROWS, 0);
  printf("\e[2K\r");
  switch (mode) {
  case ModeEx:
  case ModeCommandLine:
  case ModeReplaceChar:
    break;
  case ModeCommand:
    printf("\"%s\" ", filename);
    break;
  case ModeInsert:
    printf("-- INSERT -- ");
    break;
  }

  printf("(%d:%d/%d) ", col, (row + 1), lines);
  printf("MSG: %s", message);
  update_cursor();
}

void process_input(char c) {
  switch (mode) {
  case ModeEx:
  case ModeReplaceChar:
    break;
  case ModeCommandLine:
    commandline_mode_input(c);
    break;
  case ModeCommand:
    command_mode_input(c);
    break;
  case ModeInsert:
    insert_mode_input(c);
    break;
  }
}

void load_file(const char *pathname) {
  lines = 0;
  snprintf(filename, PATH_MAX, "%s", pathname);

  FILE *in = fopen(filename, "r");
  if (in != NULL) {
    while (fgets(buffer[lines], 256, in)) {
      int len = strlen(buffer[lines]);
      if (len > 0 && buffer[lines][len - 1] == '\n')
        buffer[lines][len - 1] = 0;
      lines++;
    }
    fclose(in);
  }

  if (lines == 0)
    ++lines;
}

void signal_handler(int signo) {
  (void)signo;
  exit(0);
}

int main(int argc, char *argv[]) {
  mode = ModeCommand;
  lines = 0;

  signal(SIGINT, signal_handler);

  load_file((argc > 1) ? argv[1] : "");

  enable_raw_mode();
  draw_screen();
  draw_status();

  char c;
  while (read(STDIN_FILENO, &c, 1) == 1) {
    process_input(c);
    if (mode == ModeCommand || mode == ModeInsert || mode == ModeReplaceChar)
      draw_status();
  }
  return 0;
}
