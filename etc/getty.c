#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, const char *argv[]) {

  if (argc != 2) {
    fprintf(stderr, "Expected TTY argument!\n");
    exit(-1);
  }

  char tty[32];
  snprintf(tty, sizeof(tty), "/dev/%s", argv[1]);

  // Close stdin, stdout & stderr
  close(2);
  close(1);
  close(0);

  // Open tty as stdin, stdout & stderr
  int fd = open(tty, O_RDWR);
  dup(fd);
  dup(fd);

  printf("\n%s\n", argv[1]);

  char login[32];
  login[0] = 0;

  while (login[0] == 0) {
    printf("\nlogin: ");
    fflush(stdout);

    if (fgets(login, sizeof(login), stdin) != NULL) {
      int len = strlen(login);
      if (len > 0)
        login[len - 1] = 0;
    }
  }

  return execl("/bin/login", "/bin/login", login, NULL);
}