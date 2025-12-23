#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

static void *curbrk = NULL;

int brk(void *addr) {
  void *result = (void *)syscall(SYS_brk, addr);
  if (result == (void *)-1) {
    return -1;
  }

  curbrk = addr;
  return 0;
}

void *sbrk(int incr) {
  if (curbrk == NULL) {
    // Get the current program break from the kernel
    curbrk = (void *)syscall(SYS_brk, NULL);
  }

  void *old = curbrk;
  if (incr > 0 || incr < 0) {
    if (brk(curbrk + incr) == -1) {
      return (void *)-1;
    }
  }

  return old;
}
