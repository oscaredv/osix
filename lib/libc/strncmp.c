#include <stddef.h>
#include <string.h>

int strncmp(const char *str1, const char *str2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (str1[i] != str2[i])
      return str1[i] - str2[i];

    if (str1[i] == 0)
      break;
  }
  return 0;
}
