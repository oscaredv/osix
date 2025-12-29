#ifndef __SYS_PIPE_H__
#define __SYS_PIPE_H__

#include <stdint.h>

struct file;

#define PIPE_BUF 1024
#define NPIPES 16

struct pipe {
  char buffer[PIPE_BUF];
  size_t read_pos;
  size_t write_pos;
  int readers;
  int writers;
};

void pipe_close(struct file *file);
ssize_t pipe_read(struct file *file, void *buf, size_t nbytes);
ssize_t pipe_write(struct file *file, const void *buf, size_t nbytes);
void pipe_init();

#endif