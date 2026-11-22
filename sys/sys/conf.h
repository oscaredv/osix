#ifndef __SYS_CONF_H__
#define __SYS_CONF_H__

#include <stddef.h>
#include <sys/inode.h>
#include <sys/types.h>

struct buf;

struct bdevsw {
  char d_name[16];
  void (*d_read)(struct buf *);
  void (*d_write)(struct buf *);
};

extern struct bdevsw bdevsw[];
extern int num_bdev;

struct cdevsw {
  char d_name[16];
  int (*d_open)(dev_t dev, int flags);
  int (*d_close)(dev_t dev);
  ssize_t (*d_read)(struct inode *inode, dev_t dev, void *buf, size_t count);
  ssize_t (*d_write)(dev_t dev, const void *buf, size_t count);
  int (*d_putchar)(dev_t dev, int c);
  int (*d_ioctl)(dev_t dev, int op, void *ptr);
  struct tty *tty;
};

extern struct cdevsw cdevsw[];
extern int num_cdev;

#endif
