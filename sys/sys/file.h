#ifndef __SYS_FILE_H__
#define __SYS_FILE_H__

#include <stddef.h>

#define NFILE 32

struct file {
  char flags;
  int ref_count;
  struct inode *inode;
  long offset;
};

extern struct file file[];

void file_init();

struct file *file_dup(struct file *file);
void file_close(struct file *file);
ssize_t file_read(struct file *file, void *buf, size_t nbytes);
ssize_t file_write(struct file *file, const void *buf, size_t nbytes);

#endif
