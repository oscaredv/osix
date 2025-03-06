#include <stdint.h>
#include <sys/syscall.h>
#include <unistd.h>

static void *curbrk = NULL;

int brk(void *addr) {
  void *result;
  asm volatile("int $0x80" : "=a"(result) : "0"(SYS_brk), "b"(addr));
  if (result == (void *)-1) {
    // TODO: errno
    return -1;
  }
  curbrk = addr;
  return 0;
}

void *sbrk(int incr) {
  if (curbrk == NULL) {
    // Get the current program break from the kernel
    asm volatile("int $0x80" : "=a"(curbrk) : "0"(SYS_brk), "b"(NULL));
  }

  void *old = curbrk;
  if (incr > 0 || incr < 0) {
    if (brk(curbrk + incr) == -1) {
      // TODO: errno
      return (void *)-1;
    }
  }

  return old;
}
