#include <errno.h>
#include <stddef.h>
#include <sys/file.h>
#include <sys/pipe.h>
#include <sys/proc.h>

ssize_t read(int fd, void *buf, size_t nbytes) {
  if (fd < 0 || fd >= NFILE || cur_proc->ofile[fd] == NULL)
    return -EBADF;

  if (cur_proc->ofile[fd]->type == FT_INODE)
    return file_read(cur_proc->ofile[fd], buf, nbytes);

  if (cur_proc->ofile[fd]->type == FT_PIPE)
    return pipe_read(cur_proc->ofile[fd], buf, nbytes);

  return -EINVAL;
}
