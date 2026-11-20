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
    "Exec format error",      // ENOEXEC
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

char *strchr(const char *s, int c) {
  while (*s) {
    if (*s == c)
      return (char *)s;
    s++;
  }
  return NULL;
}

char *strrchr(const char *s, int c) {
  const char *last = NULL;
  while (*s) {
    if (*s == c)
      last = s;
    s++;
  }
  return (char *)last;
}

char *strtok(char *str, const char *delim) {
  static char *last;
  if (str == NULL)
    str = last;

  if (str == NULL)
    return NULL;

  // Skip leading delimiters
  str += strspn(str, delim);
  if (*str == '\0') {
    last = NULL;
    return NULL;
  }

  char *token = str;
  str = strpbrk(token, delim);
  if (str) {
    *str = '\0';
    last = str + 1;
  } else {
    last = NULL;
  }
  return token;
}

size_t strspn(const char *s, const char *accept) {
  size_t count = 0;
  while (*s) {
    const char *a = accept;
    int found = 0;
    while (*a) {
      if (*s == *a) {
        found = 1;
        break;
      }
      a++;
    }
    if (!found)
      break;
    count++;
    s++;
  }
  return count;
}

char *strpbrk(const char *s, const char *accept) {
  while (*s) {
    const char *a = accept;
    while (*a) {
      if (*s == *a)
        return (char *)s;
      a++;
    }
    s++;
  }
  return NULL;
}
