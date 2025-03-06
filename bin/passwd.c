#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int main() {
  char passwd[64];
  passwd[0] = 0;

  printf("New password: ");
  fflush(stdout);

  // Disable echo
  struct termios t;
  tcgetattr(fileno(stdin), &t);
  t.c_lflag &= ~ECHO;
  tcsetattr(fileno(stdin), TCSANOW, &t);

  if (fgets(passwd, sizeof(passwd), stdin)) {
    int len = strlen(passwd);
    if (len > 0 && passwd[len - 1] == '\n')
      passwd[len - 1] = 0;

    printf("\nhash: %s\n", crypt(passwd));
  }

  // Re-enable echo
  t.c_lflag |= ECHO;
  tcsetattr(fileno(stdin), TCSANOW, &t);

  return 0;
}