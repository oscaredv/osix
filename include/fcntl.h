#ifndef __INCLUDE_FCNTL_H__
#define __INCLUDE_FCNTL_H__

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 4
#define O_APPEND 8
#define O_TRUNC 16

int open(const char *filename, int flags, ...);
int close(int fd);

#endif
