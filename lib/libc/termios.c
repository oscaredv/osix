#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <termios.h>
#include <unistd.h>

int tcgetattr(int fd, struct termios *t) { return ioctl(fd, TIOCGETA, t); }

int tcsetattr(int fd, int actions, struct termios *t) {
  (void)actions;
  return ioctl(fd, TIOCSETA, t);
}

int isatty(int fd) {
  struct termios tmp;
  return tcgetattr(fd, &tmp) == 0;
}