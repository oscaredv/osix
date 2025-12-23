#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <unistd.h>

struct stat;

int errno = 0;

static inline long syscall0(int num) {
  long ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(num));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

static inline long syscall1(int num, long b) {
  long ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(b));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

static inline long syscall2(int num, long b, long c) {
  long ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(b), "c"(c));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

static inline long syscall3(int num, long b, long c, long d) {
  long ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(num), "b"(b), "c"(c), "d"(d));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

ssize_t write(int fd, const void *buf, size_t len) {
  ssize_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_write), "b"(fd), "c"((int)buf), "d"(len));
  return ret;
}

ssize_t read(int fd, void *buf, size_t len) {
  ssize_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_read), "b"(fd), "c"((int)buf), "d"(len));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

long lseek(int fd, long offset, int whence) {
  long ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_lseek), "b"(fd), "c"(offset), "d"(whence));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

pid_t getpid() {
  pid_t pid;
  asm volatile("int $0x80" : "=a"(pid) : "0"(SYS_getpid));
  return pid;
}

pid_t getppid() {
  pid_t ppid;
  asm volatile("int $0x80" : "=a"(ppid) : "0"(SYS_getppid));
  return ppid;
}

pid_t fork() {
  pid_t child_pid;
  asm volatile("int $0x80" : "=a"(child_pid) : "0"(SYS_fork));
  return child_pid;
}

pid_t wait(int *status) {
  pid_t pid;
  asm volatile("int $0x80" : "=a"(pid) : "0"(SYS_wait), "b"(status));
  if (pid < 0) {
    errno = -pid;
    pid = -1;
  }
  return pid;
}

unsigned int sleep(unsigned int seconds) {
  unsigned int remaining;
  asm volatile("int $0x80" : "=a"(remaining) : "0"(SYS_sleep), "b"(seconds));
  return remaining;
}

int open(const char *filename, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, int);
    va_end(args);
  }

  ssize_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_open), "b"((long)filename), "c"(flags), "d"(mode));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

int close(int fd) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_close), "b"(fd));
  return ret;
}

int dup(int fd) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_dup), "b"(fd));
  return ret;
}

int chdir(const char *path) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_chdir), "b"(path));
  return ret;
}

int stat(const char *path, struct stat *st) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_stat), "b"(path), "c"(st));
  return ret;
}

int fstat(int fd, struct stat *st) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_fstat), "b"(fd), "c"(st));
  return ret;
}

int ioctl(int fd, int op, void *ptr) {
  ssize_t ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_ioctl), "b"(fd), "c"(op), "d"(ptr));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

int unlink(const char *filepath) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_unlink), "b"(filepath));
  return ret;
}

uid_t getuid() { return syscall0(SYS_getuid); }

uid_t getgid() { return syscall0(SYS_getgid); }

int setuid(uid_t uid) { return syscall1(SYS_setuid, uid); }

int setgid(gid_t gid) { return syscall1(SYS_setgid, gid); }

int setgroups(int ngroups, const gid_t *groups) { return syscall2(SYS_setgroups, ngroups, (long)groups); }

int getgroups(int size, gid_t list[]) { return syscall2(SYS_getgroups, size, (long)list); }