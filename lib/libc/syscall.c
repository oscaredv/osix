#include <stdarg.h>

int errno = 0;

long syscall(long number, ...) {
  va_list args;
  va_start(args, number);
  long ret;

  long a = number;
  long b = va_arg(args, long);
  long c = va_arg(args, long);
  long d = va_arg(args, long);

  asm volatile("int $0x80" : "=a"(ret) : "0"(a), "b"(b), "c"(c), "d"(d));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  } else {
    errno = 0;
  }

  va_end(args);
  return ret;
}
