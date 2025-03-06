#include <string.h>

// Copy src to dst, truncating or null-padding to always copy n bytes.
// Return dst.
char *strncpy(char *dst, const char *src, size_t n) {
  if (n != 0) {
    char *d = dst;

    do {
      if ((*d++ = *src++) == 0) {
        while (--n != 0) {
          *d++ = 0;
        }
        break;
      }
    } while (--n != 0);
  }
  return (dst);
}
