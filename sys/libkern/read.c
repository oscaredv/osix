#include <stddef.h>
#include <sys/file.h>
#include <sys/proc.h>

ssize_t read(int fd, void *buf, size_t nbytes) {
  if (fd < 0 || fd >= NFILE || cur_proc->ofile[fd] == NULL)
    return -1;

  return file_read(cur_proc->ofile[fd], buf, nbytes);
}
