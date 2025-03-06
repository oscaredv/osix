#ifndef __I386_KB_H__
#define __I386_KB_H__

#include <stddef.h>
#include <sys/inode.h>

void kb_init();

ssize_t kb_read(struct inode *inode, char *buf, size_t nbytes);

#endif
