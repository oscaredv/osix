#ifndef __SYS_CONF_H__
#define __SYS_CONF_H__

#include <stddef.h>
#include <sys/inode.h>
#include <sys/types.h>

struct buf;

struct bdevsw {
  char dev_name[16];
  void (*read)(struct buf *);
  void (*write)(struct buf *);
};

extern struct bdevsw bdevsw[];
extern int num_bdev;

struct cdevsw {
  char dev_name[16];
  ssize_t (*read)(struct inode *inode, dev_t dev, void *buf, size_t count);
  ssize_t (*write)(dev_t dev, const void *buf, size_t count);
  int (*putchar)(dev_t dev, int c);
  int (*ioctl)(dev_t dev, int op, void *ptr);
  struct tty *tty;
};

extern struct cdevsw cdevsw[];
extern int num_cdev;

#endif
