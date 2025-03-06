#ifndef __INCLUDE_STRING_H__
#define __INCLUDE_STRING_H__

#include <stddef.h>

size_t strlen(const char *str);

void *memmove(void *dst, const void *src, size_t size);

void *memcpy(void *dst, const void *src, size_t n);

void *memset(void *dst, int c, size_t n);

int strcmp(const char *s1, const char *s2);

int strncmp(const char *s1, const char *s2, size_t n);

char *strncpy(char *dst, const char *src, size_t n);

const char *strerror(int errnum);

char *strdup(const char *s);

#endif
