#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/syslimits.h>
#include <unistd.h>

char **environ;

int execv(const char *path, char *const argv[]) { return syscall(SYS_exec, (long)path, (long)argv, (long)environ); }

int execvp(const char *path, char *const argv[]) {
  if (*path == '/' || *path == '.') {
    return syscall(SYS_exec, (long)path, (long)argv, (long)environ);
  }

  for (int e = 0; environ[e] != NULL; e++) {
    // Look for PATH variable
    if (strncmp(environ[e], "PATH=", 5) == 0) {
      char *p = environ[e] + 5;
      char *start = p;
      while (1) {
        if (*p == ':' || *p == 0) {
          char saved = *p;
          *p = 0;

          // Construct full path
          char fullpath[PATH_MAX];
          snprintf(fullpath, sizeof(fullpath), "%s/%s", start, path);

          // Try to execute
          int ret = syscall(SYS_exec, (long)fullpath, (long)argv, (long)environ);
          if (ret != -1) {
            return ret;
          }

          *p = saved;
          if (saved == 0)
            break;
          start = p + 1;
        }
        p++;
      }
    }
  }

  return -ENOENT;
}

int execl(const char *path, const char *arg, ...) {
  // Count arguments
  va_list args;
  int argc = 1;
  va_start(args, arg);
  while (va_arg(args, char *) != NULL)
    argc++;
  va_end(args);

  char *argv[argc + 1]; // Make room for argument pointers + NULL at the end
  argv[0] = (char *)arg;

  // Build argv
  va_start(args, arg);
  for (int i = 1; i < argc; i++) {
    argv[i] = va_arg(args, char *);
  }
  va_end(args);

  // Terminate vector
  argv[argc] = NULL;

  return execv(path, argv);
}
