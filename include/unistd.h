#ifndef __INCLUDE_UNISTD_H__
#define __INCLUDE_UNISTD_H__

#include <stddef.h>
#include <sys/types.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

ssize_t write(int fd, const void *buf, size_t len);
ssize_t read(int fd, void *buf, size_t len);

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
long lseek(int fd, long offset, int whence);

int unlink(const char *filepath);

int dup(int fd);
int chdir(const char *path);

pid_t getpid(void);
pid_t getppid(void);
pid_t fork(void);
int execv(const char *path, char *const argv[]);
int execl(const char *path, const char *arg, ...);
#ifndef KERNEL
unsigned int sleep(unsigned int seconds);
#endif

int brk(void *addr);
void *sbrk(int incr);

char *crypt(const char *key);

uid_t getuid(void);
gid_t getgid(void);
int setuid(uid_t uid);
int setgid(gid_t gid);
int getgroups(int size, gid_t list[]);

long syscall(long number, ...);

#endif