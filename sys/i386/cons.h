#ifndef CONSOLE_H_
#define CONSOLE_H_

#include <stddef.h>
#include <stdint.h>
#include <sys/inode.h>
#include <sys/types.h>

void cn_init(void);

void cn_clear(void);

int cn_putchar(dev_t dev, int c);

ssize_t cn_read(struct inode *inode, dev_t dev, void *buf, size_t count);
ssize_t cn_write(dev_t dev, const void *buf, size_t count);

#endif
