#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const sys_errlist[] = {
    "No error: 0", // ENOERROR
    NULL,
    "No such file or directory", // ENOENT
    "No such process",           // ESRCH
    "Interrupted system call",   // EINTR
    NULL,
    NULL,
    NULL,
    NULL,
    "Bad file descriptor", // EBADF
    "No child processes",  // ECHILD
    NULL,
    "Cannot allocate memory", // ENOMEM
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    "No such device", // ENODEV
    NULL,
    NULL,
    "Invalid argument", // EINVAL
    NULL,
    NULL,
    "Not a terminal device" // ENOTTY
};

const int sys_nerr = sizeof(sys_errlist) / sizeof(sys_errlist[0]);

const char *strerror(int errnum) {
  if (errnum >= 0 && errnum < sys_nerr && sys_errlist[errnum] != NULL) {
    return sys_errlist[errnum];
  }

  static char buf[16];
  snprintf(buf, sizeof(buf), "Error %d", errnum);
  return buf;
}

char *strdup(const char *s) {
  if (s == NULL)
    return NULL;

  size_t len = strlen(s) + 1;
  char *dup = malloc(len);
  if (!dup)
    return NULL;

  memcpy(dup, s, len);
  return dup;
}
