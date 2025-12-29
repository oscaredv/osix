#ifndef __SYS_FILE_H__
#define __SYS_FILE_H__

#include <stddef.h>

#define NFILE 32

#define FT_INODE 0
#define FT_PIPE 1

struct pipe;

struct file {
  char flags;
  int ref_count;
  struct inode *inode;
  struct pipe *pipe;
  long offset;
  int type;
};

extern struct file file[];

void file_init();

struct file *file_dup(struct file *file);
void file_close(struct file *file);
ssize_t file_read(struct file *file, void *buf, size_t nbytes);
ssize_t file_write(struct file *file, const void *buf, size_t nbytes);
struct file *file_alloc(void);
struct proc; // Forward declaration
int file_fd_alloc(struct proc *p, struct file *f);

#endif
