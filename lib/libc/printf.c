#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

void fprintn(FILE *f, size_t *bindex, unsigned long ul, int base, char prefix, int width, int upper);

int printf(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int len = vfprintf(stdout, fmt, ap);
  va_end(ap);
  return len;
}

int fprintf(FILE *f, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  int len = vfprintf(f, fmt, ap);
  va_end(ap);
  return len;
}

FILE sntmpf; // TODO: put on stack when it doesnt have a buffer
int snprintf(char *buf, size_t bsize, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  sntmpf.ptr = buf;
  sntmpf.buf_size = bsize;
  sntmpf.fd = -1;

  int ret = vfprintf(&sntmpf, fmt, ap);
  va_end(ap);

  if (ret < 0)
    buf[0] = 0;
  else if (ret < (int)bsize)
    buf[ret] = 0;
  else
    buf[bsize - 1] = 0;

  return ret;
}

int vsnprintf(char *buf, size_t bsize, const char *fmt, va_list ap) {
  sntmpf.ptr = buf;
  sntmpf.buf_size = bsize;
  sntmpf.fd = -1;

  int ret = vfprintf(&sntmpf, fmt, ap);

  if (ret < 0)
    buf[0] = 0;
  else if (ret < (int)bsize)
    buf[ret] = 0;
  else
    buf[bsize - 1] = 0;

  return ret;
}

int vfprintf(FILE *f, const char *fmt, va_list ap) {
  unsigned long ul;
  long l;
  const char *s;

  size_t bindex = 0;

  char c = *fmt++;
  while (c != 0) {
    if (c == '%') {
      c = *fmt++;
      int lflag = 0;
      int width = 0;
      char prefix;
      if (c == 'l') {
        lflag = 1;
        c = *fmt++;
      } else if (c == '0') {
        c = *fmt++;
        prefix = '0';
        if (c >= '1' && c <= '9') {
          width = c - 0x30;
          c = *fmt++;
        }
      } else if (c >= '1' && c <= '9') {
        prefix = ' ';
        width = c - 0x30;
        c = *fmt++;
      }

      int i;
      switch (c) {
      case 'd':
        l = lflag ? va_arg(ap, long) : va_arg(ap, int);
        if (l < 0) {
          fputc('-', f);
          ++bindex;
          l = -l;
          if (width > 0) {
            --width;
          }
        }
        fprintn(f, &bindex, l, 10, prefix, width, 0);
        break;
      case 'x':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        fprintn(f, &bindex, ul, 16, prefix, width, 0);
        break;
      case 'X':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        fprintn(f, &bindex, ul, 16, prefix, width, 1);
        break;
      case 'c':
        c = va_arg(ap, int);
        fputc(c & 0x7f, f);
        ++bindex;
        break;
      case 's':
        s = va_arg(ap, char *);
        i = fputs(s, f);
        bindex += i;

        while (i++ < width) {
          fputc(' ', f);
          ++bindex;
        }
        break;
      case 'o':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        fprintn(f, &bindex, ul, 8, prefix, width, 0);
        break;
      case 'u':
        ul = lflag ? va_arg(ap, unsigned long) : va_arg(ap, unsigned int);
        fprintn(f, &bindex, ul, 10, prefix, width, 0);
        break;
      case '%':
        fputc('%', f);
        break;
      default:
        fputc('%', f);
        ++bindex;
        if (lflag) {
          fputc('l', f);
          ++bindex;
        }
        fputc(c, f);
        ++bindex;
      }
    } else {
      fputc(c, f);
      ++bindex;
    }

    c = *fmt++;
  }

  return bindex;
}

void fprintn(FILE *f, size_t *bindex, unsigned long ul, int base, char prefix, int width, int upper) {
  char tmp[(sizeof(long) * 8 / 3) + 1];
  char *p = tmp;

  do {
    if (upper) {
      *p++ = "0123456789ABCDEF"[ul % base];
    } else {
      *p++ = "0123456789abcdef"[ul % base];
    }
  } while (ul /= base);

  if (width > 0) {
    int len = p - tmp;
    while (len < width) {
      fputc(prefix, f);
      ++(*bindex);

      ++len;
    }
  }
  do {
    fputc(*--p, f);
    ++(*bindex);

  } while (p > tmp);
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
