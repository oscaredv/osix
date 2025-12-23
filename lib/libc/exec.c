#include <stdarg.h>
#include <sys/syscall.h>
#include <unistd.h>

char **environ;

int execv(const char *path, char *const argv[]) { return syscall(SYS_exec, (long)path, (long)argv, (long)environ); }

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
