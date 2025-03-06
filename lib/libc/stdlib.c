#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>

#define _NATEXIT 16

void (*_atexit_func[_NATEXIT])(void);

void exit(int status) {
  for (int f = 0; f < _NATEXIT; f++) {
    if (_atexit_func[f] != NULL) {
      _atexit_func[f]();
    }
  }

  fflush(stdout);
  asm volatile("int $0x80" : : "a"(SYS_exit), "b"(status));
}

int atexit(void (*function)(void)) {
  for (int f = 0; f < _NATEXIT; f++) {
    if (_atexit_func[f] == NULL) {
      _atexit_func[f] = function;
      return 0;
    }
  }

  errno = ENOMEM;
  return -1;
}

void memswap(void *a, void *b, size_t size) {
  char temp[size];
  memcpy(temp, a, size);
  memcpy(a, b, size);
  memcpy(b, temp, size);
}

void qsort(void *base, size_t count, size_t size, int (*compar)(const void *, const void *)) {
  if (count < 2)
    return;

  char *arr = base;
  char *pivot = arr + (count - 1) * size;
  size_t i = 0;
  for (size_t j = 0; j < count - 1; j++) {
    if (compar(arr + j * size, pivot) < 0) {
      memswap(arr + i * size, arr + j * size, size);
      i++;
    }
  }
  memswap(arr + i * size, pivot, size);

  qsort(arr, i, size, compar);
  qsort(arr + (i + 1) * size, count - i - 1, size, compar);
}
