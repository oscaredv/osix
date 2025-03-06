#include <string.h>

void *memset(void *dst, int c, size_t n) {
  unsigned char *d = dst;
  while (n != 0) {
    *d++ = c;
    n--;
  }
  return dst;
}
