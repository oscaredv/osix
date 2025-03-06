#include "vi.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void execute_shell() {
  pid_t pid = fork();
  if (pid == 0) {
    clear_screen();
    disable_raw_mode();
    execl("/bin/sh", "sh", NULL);
  } else if (pid > 0) {
    while (wait(NULL) != pid)
      ;
    enable_raw_mode();
    clear_screen();
    draw_screen();
  }
}

void run_commandline(const char *cmdline) {
  if (strcmp(cmdline, "sh") == 0 || strcmp(cmdline, "shell") == 0) {
    execute_shell();
  } else if (strncmp(cmdline, "r ", 2) == 0) {
    load_file(&cmdline[2]);
    move_cursor(0, 0);
    draw_screen();
  } else if (strcmp(cmdline, "wq") == 0) {
    clear_screen();
    save();
    quit();
  } else if (strcmp(cmdline, "w") == 0) {
    save();
  } else if (strncmp(cmdline, "w ", 2) == 0) {
    save_as(&cmdline[2]);
  } else if (strcmp(cmdline, "q") == 0 || strcmp(cmdline, "q!") == 0) {
    clear_screen();
    quit();
  }
}

void commandline_mode_input(char c) {
  static char cmdline[256];
  static int cmdlen = 0;

  if (c == '\n' || c == '\r') {
    run_commandline(cmdline);
    cmdlen = 0;
    memset(cmdline, 0, sizeof(cmdline));
    mode = ModeCommand;
  } else if (c == 27) { // ESC
    mode = ModeCommand;
    cmdlen = 0;
    memset(cmdline, 0, sizeof(cmdline));
  } else if (c == 127) { // Backspace/DEL
    if (cmdlen > 0) {
      cmdline[--cmdlen] = 0;
      printf("\b \b");
    }
  } else if (isprint(c)) {
    cmdline[cmdlen++] = c;
    printf("%c", c);
  }
  fflush(stdout);
}