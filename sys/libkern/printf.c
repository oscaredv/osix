#include <i386/cons.h>
#include <stdarg.h>
#include <stdio.h>

void printn(unsigned long ul, int base, int width, int upper);
int putchar(int c) { return cn_write(0, &c, 1); }

int printf(const char *fmt, ...) {
  unsigned long ul;
  long l;
  const char *s;
  va_list ap;
  va_start(ap, fmt);

  char c = *fmt++;
  while (c != 0) {
    if (c == '%') {
      c = *fmt++;
      int lflag = 0;
      int width = 0;
      if (c == 'l') {
        lflag = 1;
        c = *fmt++;
      } else if (c == '0') {
        c = *fmt++;
        if (c >= '1' && c <= '9') {
          width = c - 0x30;
          c = *fmt++;
        }
      }

      switch (c) {
      case 'd':
        l = lflag ? va_arg(ap, long) : va_arg(ap, int);
        if (l < 0) {
          putchar('-');
          l = -l;
          if (width > 0) {
            --width;
          }
        }
        printn(l, 10, width, 0);
        break;
      case 'x':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        printn(ul, 16, width, 0);
        break;
      case 'X':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        printn(ul, 16, width, 1);
        break;
      case 'c':
        c = va_arg(ap, int);
        putchar(c & 0x7f);
        break;
      case 's':
        s = va_arg(ap, char *);
        printf(s);
        break;
      case 'o':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        printn(ul, 8, width, 0);
        break;
      case 'u':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        printn(ul, 10, width, 0);
        break;
      case '%':
        putchar('%');
        break;
      default:
        putchar('%');
        if (lflag) {
          putchar('l');
        }
        putchar(c);
      }
    } else {
      putchar(c);
    }

    c = *fmt++;
  }
  va_end(ap);
  return 0;
}

void printn(unsigned long ul, int base, int width, int upper) {
  char buf[(sizeof(long) * 8 / 3) + 1];
  char *p = buf;

  do {
    if (upper) {
      *p++ = "0123456789ABCDEF"[ul % base];
    } else {
      *p++ = "0123456789abcdef"[ul % base];
    }
  } while (ul /= base);

  if (width > 0) {
    int len = p - buf;
    while (len < width) {
      putchar('0');
      ++len;
    }
  }
  do {
    putchar(*--p);
  } while (p > buf);
}

void panic(const char *msg) {
  printf("panic: %s\n", msg);
  asm("cli");
  asm("hlt");
}

void hexdump(const void *p, unsigned int len) {
  unsigned char *ptr = (unsigned char *)p;
  int x = 0;
  for (unsigned int i = 0; i < len; i++) {
    printf("%02x ", ptr[i]);
    x++;
    if (x >= 32) {
      printf("\n");
      x = 0;
    }
  }
  printf("\n");
}
