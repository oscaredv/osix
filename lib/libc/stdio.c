#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#define _NFILE 8
FILE _iob[_NFILE];

FILE *falloc() {
  for (int i = 0; i < _NFILE; i++) {
    if (_iob[i].flags == 0) {
      _iob[i].flags = _IOREAD;
      _iob[i].size = 0;
      _iob[i].buf_size = sizeof(_iob[i].buf);
      _iob[i].ptr = _iob[i].buf;
      return &_iob[i];
    }
  }
  return NULL;
}

void ffree(FILE *f) { f->flags = 0; }

FILE *fopen(const char *filename, const char *mode) {
  FILE *f = falloc();
  if (f == NULL)
    return NULL;

  int flags = 0;
  while (*mode) {
    switch (*mode++) {
    case 'r':
      flags = O_RDONLY;
      break;
    case 'w':
      flags = O_WRONLY | O_CREAT | O_TRUNC;
      break;
    case 'a':
      flags = O_WRONLY | O_CREAT | O_APPEND;
      break;
    case 'b':
      break;
    case '+':
      if (flags & O_RDONLY)
        flags = (flags & ~O_RDONLY) | O_RDWR;
      else if (flags & O_WRONLY)
        flags = (flags & ~O_WRONLY) | O_RDWR;
      break;
    }
  }

  f->fd = open(filename, flags, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
  if (f->fd == -1) {
    ffree(f);
    return NULL;
  }

  f->size = 0;
  return f;
}

FILE *fdopen(int fd, const char *mode) {
  (void)mode;
  FILE *f = falloc();
  if (f == NULL)
    return NULL;

  f->fd = fd;
  return f;
}

int fclose(FILE *f) {
  if (f->flags == 0)
    return EOF;

  // TODO: write?
  close(f->fd);
  f->fd = -1;
  f->flags = 0;
  return 0;
}

void _filebuf(FILE *f) {
  f->ptr = f->buf;
  f->size = 0;
  int s = 0;
  do {
    s = read(f->fd, f->ptr, f->buf_size);
  } while (s == -1 && errno == EINTR);
  if (s > 0)
    f->size = s;
  if (s == 0)
    f->flags |= _IOEOF;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *f) {
  size_t total = size * count;
  size_t nbytes = 0;

  while (total > 0) {
    if (f->size == 0) {
      if (f->flags & _IOEOF) {
        return 0;
      }
      fflush(f);
      _filebuf(f);
      if (f->size == 0) {
        break;
      }
    }
    size_t segment = MIN(total, f->size);
    memcpy(ptr, f->ptr, segment);

    f->size -= segment;
    f->ptr += segment;
    ptr += segment;
    nbytes += segment;
    total -= segment;
  }

  return nbytes / size;
}

size_t fwrite(const void *src, size_t size, size_t count, FILE *f) {
  size_t total = size * count;
  size_t nbytes = 0;

  while (total > 0) {
    if (f->size >= sizeof(f->buf)) {
      fflush(f);
    }
    size_t segment = MIN(total, sizeof(f->buf));
    memcpy(f->ptr, src, segment);

    f->size += segment;
    f->ptr += segment;
    src += segment;
    nbytes += segment;
    total -= segment;
  }

  return nbytes / size;
}

int getc(FILE *f) { return fgetc(f); }

int getchar(void) { return getc(stdin); }

int fgetc(FILE *f) {
  unsigned char c;
  size_t len = fread(&c, 1, 1, f);
  if (len != 1)
    return EOF;

  return c;
}

char *fgets(char *dst, size_t size, FILE *f) {
  if (size == 0)
    return NULL;

  size_t i = 0;
  char c = 0;
  while (c != '\n' && i < size - 1) {
    if (f->size == 0) {
      _filebuf(f);
      if (f->size == 0) {
        break;
      }
    }
    c = *f->ptr++;
    --f->size;
    dst[i++] = c;
  }
  dst[i] = 0;

  return (i > 0) ? dst : NULL;
}

int fflush(FILE *f) {
  if (f->fd == -1)
    return 0; // TODO: EOF?

  size_t size = f->size;
  f->ptr = f->buf;
  f->size = 0;

  ssize_t len = write(f->fd, f->ptr, size);

  if (len != (ssize_t)size)
    return EOF;

  return 0;
}

int fileno(FILE *f) { return f->fd; }

int fputc(int c, FILE *f) {
  if (f->size >= f->buf_size) {
    if (fflush(f) != 0)
      return EOF;
  }

  *f->ptr++ = c;
  ++f->size;

  if (c == '\n') { // TODO: only terminals should flush on newline
    if (fflush(f) != 0)
      return EOF;
  }

  return c;
}

int fputs(const char *s, FILE *f) {
  if (!s)
    s = "(null)";

  int c = 0;
  while (s[c] != 0) {
    if (fputc(s[c++], f) < 0)
      return EOF;
  }
  return c;
}

void perror(const char *s) {
  int e = errno;
  if (s != NULL && *s != 0) {
    printf("%s: ", s);
  }

  printf("%s\n", strerror(e));
}

void stdio_init() {
  fdopen(STDIN_FILENO, "r");
  fdopen(STDOUT_FILENO, "w");
  fdopen(STDERR_FILENO, "w");
}
