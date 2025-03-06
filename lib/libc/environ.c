#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char **environ = NULL;

char *getenv(const char *name) {
  if (!name || *name == 0) {
    errno = EINVAL;
    return NULL;
  }
  int name_len = strlen(name);

  // Name cannot contain '='
  for (int i = 0; i < name_len; i++) {
    if (name[i] == '=') {
      errno = EINVAL;
      return NULL;
    }
  }

  char **e = environ;
  while (*e != NULL) {
    if (strncmp(name, *e, name_len) == 0 && e[0][name_len] == '=') {
      return (*e) + name_len + 1;
    }
    ++e;
  }
  return NULL;
}

int setenv(const char *name, const char *value, int overwrite) {
  if (!name || *name == 0 || !value) {
    errno = EINVAL;
    return -1;
  }

  int name_len = strlen(name);
  int value_len = strlen(value);

  // Name cannot contain '='
  for (int i = 0; i < name_len; i++) {
    if (name[i] == '=') {
      errno = EINVAL;
      return -1;
    }
  }

  int e = 0;
  while (environ[e] != NULL) {
    if (strncmp(name, environ[e], name_len) == 0 && environ[e][name_len] == '=') {
      if (overwrite == 0)
        return 0;

      char *old_value = &environ[e][name_len + 1];
      int old_len = strlen(old_value);
      if (value_len <= old_len) {
        strncpy(old_value, value, old_len + 1);
      } else {
        int total = name_len + value_len + 2;
        char *new_var = malloc(total);
        if (!new_var) {
          errno = ENOMEM;
          return -1;
        }
        snprintf(new_var, total, "%s=%s", name, value);

        // Free old variable, but only if it's memory was stored on the heap
        if ((char *)&total > environ[e])
          free(environ[e]);

        environ[e] = new_var;
      }
      return 0;
    }
    ++e;
  }

  char **new_environ = malloc((e + 2) * sizeof(char *));
  if (!new_environ) {
    errno = ENOMEM;
    return -1;
  }
  for (int i = 0; i < e; i++) {
    new_environ[i] = environ[i];
  }
  new_environ[e + 1] = NULL;

  int total = name_len + value_len + 2;
  new_environ[e] = malloc(total);
  if (!new_environ[e]) {
    free(new_environ);
    errno = ENOMEM;
    return -1;
  }
  snprintf(new_environ[e], total, "%s=%s", name, value);

  // Free old vector, but only if it's memory is stored on the heap
  if ((char **)&total > environ)
    free(environ);

  environ = new_environ;

  return 0;
}

int unsetenv(const char *name) {
  if (!name || *name == 0) {
    errno = EINVAL;
    return -1;
  }
  int name_len = strlen(name);

  // Name cannot contain '='
  for (int i = 0; i < name_len; i++) {
    if (name[i] == '=') {
      errno = EINVAL;
      return -1;
    }
  }

  int e = 0;
  while (environ[e] != NULL) {
    if (strncmp(name, environ[e], name_len) == 0 && environ[e][name_len] == '=') {
      int i = e;

      // Free variable we're unsetting, but only if it's memory is stored on the heap
      if ((char *)&i > environ[e])
        free(environ[e]);

      while (environ[i] != NULL) {
        environ[i] = environ[i + 1];
        ++i;
      }
      return 0;
    }
    ++e;
  }
  return 0;
}
