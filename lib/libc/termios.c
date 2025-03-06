#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>

int tcgetattr(int fd, struct termios *t) { return ioctl(fd, TIOCGETA, t); }

int tcsetattr(int fd, int actions, struct termios *t) {
  (void)actions;
  return ioctl(fd, TIOCSETA, t);
}
