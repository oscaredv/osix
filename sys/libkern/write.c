#include <stddef.h>
#include <sys/file.h>
#include <sys/proc.h>

ssize_t write(int fd, const void *buf, size_t nbytes) {
  if (fd < 0 || fd >= NFILE || cur_proc->ofile[fd] == NULL) {
    return -1;
  }

  return file_write(cur_proc->ofile[fd], buf, nbytes);
}
