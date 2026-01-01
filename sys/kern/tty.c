#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/system.h>
#include <sys/tty.h>

struct tty ttys[NTTY];

void tty_init() {
  for (int t = 0; t < NTTY; t++) {
    ttys[t].termios.c_lflag = ECHO | ICANON;
  }
}

ssize_t tty_read(struct inode *inode, dev_t dev, void *buf, size_t count) {
  if (major(dev) >= num_cdev)
    return -ENODEV;

  struct tty *tty = &cdevsw[major(dev)].tty[minor(dev)];

  // Remember session's process group
  tty->pgrp = cur_proc->pgrp;

  char *dest = buf;
  char c = 0;
  size_t i = 0;

  while (i < count && c != '\r' && c != '\n') {
    while (tty->rx_head == tty->rx_tail) {
      if (signal_pending(cur_proc))
        return -EINTR;

      iunlock(inode);
      sleep(tty);
      ilock(inode);
    }
    c = tty->rx_buf[tty->rx_tail++];

    if ((tty->termios.c_lflag & ICANON) == 0) {
      dest[i++] = c;
      break;
    }

    if (c == '\r')
      c = '\n';

    if (c == 127 || c == 8) { // backspace or CTRL-h
      if (i > 0) {
        if (tty->termios.c_lflag & ECHO)
          tty_write(dev, "\b \b", 3);
        dest[--i] = 0;
      }
    } else if (c == 23) { // CTRL-w: kill word
      if (i > 0) {
        if (tty->termios.c_lflag & ECHO)
          tty_write(dev, "\b \b", 3);
        --i;
      }
      while (i > 0 && dest[i] == ' ') {
        if (tty->termios.c_lflag & ECHO)
          tty_write(dev, "\b \b", 3);
        --i;
      }
      while (i > 0 && dest[i] != ' ') {
        if (tty->termios.c_lflag & ECHO)
          tty_write(dev, "\b \b", 3);
        --i;
      }
    } else if (c == 21) { // CTRL-u: kill line
      for (size_t k = 0; k < i; k++) {
        if (tty->termios.c_lflag & ECHO)
          tty_write(dev, "\b \b", 3);
      }
      i = 0;
    } else if (c == 12) { // CTRL-l: clear screen
      if (i == 0) {
        if (tty->termios.c_lflag & ECHO) {
          tty_write(dev, "\e[H", 3);  // Cursor home
          tty_write(dev, "\e[2J", 4); // Clear screen
        }
        break;
      }
    } else if (c == 4) { // EOT
      return EOF;
    } else { // Echo all other characters
      if (tty->termios.c_lflag & ECHO)
        tty_write(dev, &c, 1);
      dest[i++] = c;
    }
  }

  return i;
}

ssize_t tty_write(dev_t dev, const void *buf, size_t count) {
  if (major(dev) >= num_cdev || cdevsw[major(dev)].putchar == NULL)
    return -ENODEV;

  const char *p = buf;
  for (size_t i = 0; i < count; i++) {
    int len = cdevsw[major(dev)].putchar(dev, *p++);
    if (len < 0)
      return len;
  }

  return count;
}

void tty_input(dev_t dev, char c) {
  if (minor(dev) >= NTTY)
    return;

  struct tty *tty = &cdevsw[major(dev)].tty[minor(dev)];
  if (c == 3) {
    // CTRL-c pressed - kill session's process group
    if (tty->pgrp > 1)
      kill(-tty->pgrp, SIGINT);
  } else {
    // Store in input ring-buffer
    tty->rx_buf[tty->rx_head++] = c;
    if (tty->rx_head == tty->rx_tail) {
      tty->rx_tail++;
    }
    wakeup(tty);
  }
}

int tty_ioctl(dev_t dev, int op, void *ptr) {
  // major(dev) is range checked in ioctl()
  if (minor(dev) < 0 || minor(dev) >= NTTY || cdevsw[major(dev)].tty == NULL)
    return -ENOTTY;

  if (ptr == NULL)
    return -EFAULT;

  struct tty *tty = &cdevsw[major(dev)].tty[minor(dev)];
  struct termios *termios = ptr;
  int ret = 0;
  if (op == TIOCGETA) {
    *termios = tty->termios;
  } else if (op == TIOCSETA) {
    tty->termios = *termios;
  } else {
    ret = -EINVAL;
  }
  return ret;
}
