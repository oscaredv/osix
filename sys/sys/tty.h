#ifndef __SYS_TTY_H__
#define __SYS_TTY_H__

#include <stddef.h>
#include <sys/types.h>
#include <termios.h>

#define NTTY 3

struct tty {
  char rx_buf[256];
  volatile uint8_t rx_head, rx_tail;
  short pgrp;
  struct termios termios;
};

extern struct tty ttys[NTTY];

ssize_t tty_read(struct inode *inode, dev_t dev, void *buf, size_t count);
ssize_t tty_write(dev_t dev, const void *buf, size_t count);

void tty_input(dev_t dev, char c);

int tty_ioctl(dev_t dev, int op, void *ptr);

void tty_init();

#endif
