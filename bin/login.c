#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

int main(int argc, const char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Expected user name\n");
    exit(EXIT_FAILURE);
  }

  char password[32];
  password[0] = 0;

  // Disable echo
  struct termios t;
  tcgetattr(fileno(stdin), &t);
  t.c_lflag &= ~ECHO;
  tcsetattr(fileno(stdin), TCSANOW, &t);

  while (password[0] == 0) {
    printf("password: ");
    fflush(stdout);

    if (fgets(password, sizeof(password), stdin) != NULL) {
      int len = strlen(password);
      if (len > 0)
        password[len - 1] = 0;
    }
    printf("\n");
  }

  // Re-enable echo
  t.c_lflag |= ECHO;
  tcsetattr(fileno(stdin), TCSANOW, &t);

  // Check if user exists
  struct passwd *pw = getpwnam(argv[1]);
  if (pw == NULL) {
    exit(EXIT_FAILURE);
  }

  // Check if password matches passwd file
  char *hash = crypt(password);
  if (strcmp(hash, pw->pw_passwd) != 0) {
    exit(EXIT_FAILURE);
  }

  // Set gid
  if (setgid(pw->pw_gid)) {
    exit(EXIT_FAILURE);
  }

  // Set supplementary groups
  initgroups(pw->pw_name, pw->pw_gid);

  // Set uid
  if (setuid(pw->pw_uid)) {
    exit(EXIT_FAILURE);
  }

  // Write /etc/motd to terminal
  FILE *motd = fopen("/etc/motd", "r");
  if (motd) {
    char line[256];
    while (fgets(line, sizeof(line), motd)) {
      fprintf(stdout, "%s", line);
    }
    fclose(motd);
  }

  // Change to users home directory
  chdir(pw->pw_dir);

  // Execute users login shell
  return execl(pw->pw_shell, pw->pw_shell, "-", NULL);
}
