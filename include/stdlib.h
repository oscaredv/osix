#ifndef __STDLIB_H__
#define __STDLIB_H__

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#ifndef __SIZE_T__
#define __SIZE_T__
typedef unsigned int size_t;
typedef int ssize_t;
#endif

void exit(int status);
int atexit(void (*function)(void));

int atoi(const char *s);

void *malloc(size_t size);
void free(void *ptr);

void qsort(void *base, size_t count, size_t size, int (*compar)(const void *, const void *));

char *getenv(const char *name);
int setenv(const char *name, const char *value, int overwrite);
int unsetenv(const char *name);

#endif
