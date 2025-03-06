#include <i386/cons.h>
#include <i386/uart.h>
#include <i386/wd.h>
#include <string.h>
#include <sys/conf.h>
#include <sys/tty.h>

struct bdevsw bdevsw[] = {{"wd", wd_read, wd_write}};

int num_bdev = sizeof(bdevsw) / sizeof(struct bdevsw);

ssize_t zero_read(struct inode *inode, dev_t dev, void *buf, size_t count) {
  (void)inode;
  (void)dev;
  memset(buf, 0, count);
  return count;
}

ssize_t null_read(struct inode *inode, dev_t dev, void *buf, size_t count) {
  (void)inode;
  (void)dev;
  (void)buf;
  (void)count;
  return 0;
}

ssize_t null_write(dev_t dev, const void *buf, size_t count) {
  (void)dev;
  (void)buf;
  (void)count;
  return 0;
}

struct cdevsw cdevsw[] = {{"null", null_read, null_write, NULL, NULL, NULL},
                          {"console", tty_read, tty_write, cn_putchar, tty_ioctl, &ttys[0]},
                          {"zero", zero_read, null_write, NULL, NULL, NULL},
                          {"tty", tty_read, tty_write, uart_putchar, tty_ioctl, &ttys[1]}};

int num_cdev = sizeof(cdevsw) / sizeof(struct cdevsw);
