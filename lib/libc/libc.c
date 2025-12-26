#include <fcntl.h>
#include <stdarg.h>
#include <sys/syscall.h>
#include <unistd.h>

struct stat;

ssize_t write(int fd, const void *buf, size_t len) { return syscall(SYS_write, fd, (long)buf, len); }

ssize_t read(int fd, void *buf, size_t len) { return syscall(SYS_read, fd, (long)buf, len); }

long lseek(int fd, long offset, int whence) { return syscall(SYS_lseek, fd, offset, whence); }

pid_t getpid(void) { return syscall(SYS_getpid); }

pid_t getppid(void) { return syscall(SYS_getppid); }

pid_t fork(void) { return syscall(SYS_fork); }

pid_t wait(int *status) { return syscall(SYS_wait, (long)status); }

unsigned int sleep(unsigned int seconds) { return syscall(SYS_sleep, seconds); }

int open(const char *filename, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, int);
    va_end(args);
  }

  return syscall(SYS_open, (long)filename, flags, mode);
}

int close(int fd) { return syscall(SYS_close, fd); }

int dup(int fd) { return syscall(SYS_dup, fd); }

int chdir(const char *path) { return syscall(SYS_chdir, (long)path); }

int stat(const char *path, struct stat *st) { return syscall(SYS_stat, (long)path, (long)st); }

int fstat(int fd, struct stat *st) { return syscall(SYS_fstat, fd, (long)st); }

int ioctl(int fd, int op, void *ptr) { return syscall(SYS_ioctl, fd, op, (long)ptr); }

int unlink(const char *filepath) { return syscall(SYS_unlink, (long)filepath); }

uid_t getuid() { return syscall(SYS_getuid); }

uid_t getgid() { return syscall(SYS_getgid); }

int setuid(uid_t uid) { return syscall(SYS_setuid, uid); }

int setgid(gid_t gid) { return syscall(SYS_setgid, gid); }

int setgroups(int ngroups, const gid_t *groups) { return syscall(SYS_setgroups, ngroups, (long)groups); }

int getgroups(int size, gid_t list[]) { return syscall(SYS_getgroups, size, (long)list); }
