#include <stdint.h>
#include <string.h>

void *memmove(void *dst, const void *src, size_t n) {
  uint8_t *d = (uint8_t *)dst;
  const uint8_t *s = (const uint8_t *)src;

  if (d < s) {
    for (size_t i = 0; i < n; i++) {
      d[i] = s[i];
    }
  } else if (d > s) {
    do {
      --n;
      d[n] = s[n];
    } while (n > 0);
  }

  return dst;
}