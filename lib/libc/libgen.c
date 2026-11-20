#include <string.h>
#include <sys/syslimits.h>

char *basename(char *path) {
  int len = strlen(path);
  while (len > 0 && path[len - 1] == '/')
    path[--len] = 0;

  char *base = strrchr(path, '/');
  if (!base)
    return path;

  return base + 1;
}

char *dirname(char *path) {
  int len = strlen(path);
  while (len > 0 && path[len - 1] == '/')
    path[--len] = 0;

  char *end = strrchr(path, '/');
  if (!end)
    return ".";

  *++end = 0;
  return path;
}
