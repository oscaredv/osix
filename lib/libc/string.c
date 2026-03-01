#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *const sys_errlist[] = {
    "No error: 0",               // ENOERROR
    "Operation not permitted",   // EPERM
    "No such file or directory", // ENOENT
    "No such process",           // ESRCH
    "Interrupted system call",   // EINTR
    "Input/output error",        // EIO
    NULL,
    NULL,
    NULL,
    "Bad file descriptor",    // EBADF
    "No child processes",     // ECHILD
    "Try again",              // EAGAIN
    "Cannot allocate memory", // ENOMEM
    "Permission denied",      // EACCES
    "Bad address",            // EFAULT
    NULL,
    NULL,
    "File exists", // EEXIST
    NULL,
    "No such device",  // ENODEV
    "Not a directory", // ENOTDIR
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
