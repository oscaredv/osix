#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/pipe.h>
#include <sys/proc.h>
#include <sys/system.h>

struct pipe pipes[NPIPES];

struct pipe *pipe_create() {
  for (int i = 0; i < NPIPES; i++) {
    if (pipes[i].readers == 0 && pipes[i].writers == 0) {
      return &pipes[i];
    }
  }
  return NULL;
}

void pipe_destroy(struct pipe *p) { memset(p, 0, sizeof(struct pipe)); }

ssize_t pipe_read(struct file *file, void *buf, size_t nbytes) {
  if (file->type != FT_PIPE || file->pipe == NULL)
    panic("pipe_read: not a pipe");

  struct pipe *p = file->pipe;
  size_t bytes_read = 0;

  while (bytes_read < nbytes) {
    if (p->read_pos == p->write_pos) {
      if (p->writers == 0 || bytes_read > 0) {
        wakeup(p);
        return bytes_read;
      }
      // No data available - block
      sleep(p);
    } else {
      ((char *)buf)[bytes_read++] = p->buffer[p->read_pos % sizeof(p->buffer)];
      p->read_pos++;
    }
  }
  wakeup(p);
  return bytes_read;
}

ssize_t pipe_write(struct file *file, const void *buf, size_t nbytes) {
  if (file->type != FT_PIPE || file->pipe == NULL)
    panic("pipe_write: not a pipe");

  struct pipe *p = file->pipe;
  size_t bytes_written = 0;

  while (bytes_written < nbytes) {
    if ((p->write_pos - p->read_pos) == sizeof(p->buffer)) {
      //  Buffer full, wake up readers then block
      wakeup(p);
      sleep(p);
    } else {
      p->buffer[p->write_pos % sizeof(p->buffer)] = ((const char *)buf)[bytes_written++];
      p->write_pos++;
    }
  }
  wakeup(p);
  return bytes_written;
}

void pipe_close(struct file *file) {
  if (file->type != FT_PIPE || file->pipe == NULL)
    panic("pipe_close: not a pipe");

  struct pipe *p = file->pipe;
  if (--file->ref_count == 0) {
    if (file->flags & O_RDONLY) {
      p->readers--;
    }
    if (file->flags & O_WRONLY) {
      p->writers--;
    }
  }

  if (p->readers == 0 && p->writers == 0) {
    pipe_destroy(p);
  }
}

int sys_pipe(int fd[2]) {
  if ((long)fd < USER_TEXT || (long)fd >= (USER_USTACK + STACK_SIZE))
    return -EFAULT;

  struct file *read_end = file_alloc();
  struct file *write_end = file_alloc();
  if (read_end == NULL || write_end == NULL) {
    if (read_end)
      file_close(read_end);
    if (write_end)
      file_close(write_end);
    return -ENFILE;
  }

  read_end->flags = O_RDONLY;
  write_end->flags = O_WRONLY;

  struct pipe *p = pipe_create();
  if (p == NULL) {
    file_close(read_end);
    file_close(write_end);
    return -ENFILE;
  }

  read_end->type = FT_PIPE;
  read_end->pipe = p;
  read_end->pipe->readers++;
  write_end->type = FT_PIPE;
  write_end->pipe = p;
  write_end->pipe->writers++;
  int read_fd = file_fd_alloc(cur_proc, read_end);
  int write_fd = file_fd_alloc(cur_proc, write_end);
  if (read_fd < 0 || write_fd < 0) {
    if (read_fd >= 0)
      cur_proc->ofile[read_fd] = NULL;
    if (write_fd >= 0)
      cur_proc->ofile[write_fd] = NULL;
    file_close(read_end);
    file_close(write_end);
    pipe_destroy(p);
    return -EMFILE;
  }
  fd[0] = read_fd;
  fd[1] = write_fd;

  return 0;
}

void pipe_init() { memset(pipes, 0, sizeof(pipes)); }
