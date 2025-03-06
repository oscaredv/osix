#include <fcntl.h>
#include <pwd.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static char *passwd_file = NULL;
static size_t passwd_count = 0;
static struct passwd *passwds = NULL;

int parsepwd() {
  int fd = open("/etc/passwd", O_RDONLY);
  struct stat st;
  if (fstat(fd, &st) == -1 || st.st_size == 0) {
    close(fd);
    return -1;
  }

  passwd_file = malloc(st.st_size + 1);
  if (passwd_file == NULL) {
    // TODO: ENOMEM
    close(fd);
    return -1;
  }

  // Load password file
  if (read(fd, passwd_file, st.st_size) != st.st_size) {
    close(fd);
    return -1;
  }
  passwd_file[st.st_size] = 0;
  close(fd);

  // Count entries
  char *p = passwd_file;
  passwd_count = 0;
  while (*p != 0) {
    if (*p++ == '\n') // Count newlines
      ++passwd_count;
    else if (*p == 0) // Count entry even if newline is missing
      ++passwd_count;
  }

  passwds = malloc(passwd_count * sizeof(struct passwd));
  if (passwds == NULL) {
    free(passwd_file);
    passwd_file = NULL;
    return -1; // TODO: ENOMEM
  }

  p = passwd_file;
  for (size_t e = 0; e < passwd_count; e++) {
    char *fields[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
    for (int f = 0; f < 7; f++) {
      fields[f] = p;
      while (*p != ':' && *p != '\n' && *p != 0)
        ++p;

      char op = *p;
      *p++ = 0;
      if (op == '\n' || op == 0) {
        break;
      }
    }

    passwds[e].pw_name = fields[0];
    passwds[e].pw_passwd = fields[1];
    passwds[e].pw_uid = atoi(fields[2]);
    passwds[e].pw_gid = atoi(fields[3]);
    passwds[e].pw_gecos = fields[4];
    passwds[e].pw_dir = fields[5];
    passwds[e].pw_shell = fields[6];
  }

  return 0;
}

struct passwd *getpwuid(short uid) {
  if (passwds == NULL && parsepwd() == -1)
    return NULL;

  for (size_t i = 0; i < passwd_count; i++) {
    if (passwds[i].pw_uid == uid) {
      return &passwds[i];
    }
  }

  return NULL;
}

struct passwd *getpwnam(const char *name) {
  if (passwds == NULL && parsepwd() == -1)
    return NULL;

  for (size_t i = 0; i < passwd_count; i++) {
    if (strcmp(passwds[i].pw_name, name) == 0) {
      return &passwds[i];
    }
  }

  return NULL;
}
