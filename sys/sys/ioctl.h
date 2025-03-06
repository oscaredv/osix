#ifndef __SYS_IOCTL_H__
#define __SYS_IOCTL_H__

enum { TIOCGETA = 1, TIOCSETA };

int ioctl(int fd, int op, void *ptr);

#endif
