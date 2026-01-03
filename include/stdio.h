#ifndef __INCLUDE_STDIO_H__
#define __INCLUDE_STDIO_H__

#include <stdarg.h>
#include <stddef.h>

#define EOF (-1)
#define BUFSIZ 1024

#define _IOREAD 1
#define _IOWRT 2
#define _IOEOF 4

typedef struct {
  int fd;
  char buf[1024];
  size_t size;
  size_t buf_size;
  char *ptr;
  char flags;
} FILE;

extern FILE _iob[];

FILE *fopen(const char *filename, const char *mode);
FILE *fdopen(int fd, const char *mode);
int fflush(FILE *f);
int fclose(FILE *f);
size_t fread(void *dst, size_t size, size_t count, FILE *f);
size_t fwrite(const void *src, size_t size, size_t count, FILE *f);

char *fgets(char *dst, size_t size, FILE *f);
int fgetc(FILE *f);
int getc(FILE *f);
int getchar(void);

int fputc(int c, FILE *f);
int fputs(const char *s, FILE *f);

int printf(const char *fmt, ...);
int fprintf(FILE *f, const char *fmt, ...);
int vfprintf(FILE *f, const char *fmt, va_list ap);

int snprintf(char *buf, size_t bsize, const char *fmt, ...);
int vsnprintf(char *buf, size_t bsize, const char *fmt, va_list ap);

int fileno(FILE *f);

void perror(const char *s);

void hexdump(const void *p, unsigned int len);

#define stdin (&_iob[0])
#define stdout (&_iob[1])
#define stderr (&_iob[2])

#endif
