#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/inode.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/system.h>
#include <unistd.h>

struct file file[NFILE];

void file_init() { memset(file, 0, sizeof(file)); }

int file_fd_alloc(struct proc *p, struct file *f) {
  for (int i = 0; i < NFILE; i++) {
    if (p->ofile[i] == NULL) {
      p->ofile[i] = f;
      return i;
    }
  }
  return -1;
}

struct file *file_alloc(void) {
  for (int f = 0; f < NFILE; f++) {
    if (file[f].ref_count == 0) {
      memset(&file[f], 0, sizeof(struct file));
      ++file[f].ref_count;
      return &file[f];
    }
  }

  return NULL;
}

void file_close(struct file *file) {
  if (file->ref_count <= 0)
    panic("file_close");

  if (--file->ref_count == 0) {
    if (file->inode) {
      iput(file->inode);
      file->inode = NULL;
    }
    // File is no more...
  }
}

int close(int fd) {
  if (fd < 0 || fd >= NFILE || cur_proc->ofile[fd] == NULL)
    return -1;

  file_close(cur_proc->ofile[fd]);
  cur_proc->ofile[fd] = NULL;
  return 0;
}

void wdir(struct inode *dir, unsigned int inodeno, const char *filename) {
  size_t offset = 0;
  struct dirent entry;

  ilock(dir);
  while (readi(dir, &entry, offset, sizeof(entry)) == sizeof(entry)) {
    if (entry.d_ino == 0) {
      break;
    }
    offset += sizeof(entry);
  }

  entry.d_ino = inodeno;
  strncpy(entry.d_name, filename, sizeof(entry.d_name));
  entry.d_name[sizeof(entry.d_name) - 1] = 0;
  writei(dir, &entry, offset, sizeof(entry));

  long end = offset + sizeof(entry);
  if (end > dir->i_size) {
    dir->i_size = end;
    dir->i_flags |= I_UPDATED;
  }
  iunlock(dir);
}

int sys_unlink(const char *filepath) {
  char filename[PATH_MAX];
  struct inode *parent = parenti(filepath, filename);
  if (!parent) {
    return -ENOENT;
  }

  if ((parent->i_mode & S_IFMT) != S_IFDIR) {
    return -ENOENT; // TODO: not dir
  }
  struct dirent entry;
  long offset = 0;
  ilock(parent);
  while (readi(parent, &entry, offset, sizeof(entry)) == sizeof(entry)) {
    if (entry.d_ino != 0 && strcmp(entry.d_name, filename) == 0) {
      unsigned int inodeno = entry.d_ino;
      entry.d_ino = 0;
      *entry.d_name = 0;
      writei(parent, &entry, offset, sizeof(entry));
      iunlockput(parent);

      struct inode *inode = iget(parent->dev, inodeno);
      if (!inode)
        panic("unlink: inode==NULL");

      ilock(inode);
      if (inode->i_nlinks == 0)
        panic("unlink: nlinks==0");

      --inode->i_nlinks;
      inode->i_flags |= I_UPDATED;
      iunlockput(inode);
      return 0;
    }
    offset += sizeof(entry);
  }
  return -ENOENT;
}

struct inode *creat(const char *filename, int mode) {
  struct inode *inode = ialloc(mode);
  if (inode == NULL)
    panic("No free inode");

  char base[PATH_MAX];
  struct inode *parent = parenti(filename, base);
  if (!parent)
    panic("wdir");

  ilock(inode);
  wdir(parent, inode->i_inodeno, base);

  inode->i_nlinks += 1;
  iunlock(inode);
  iput(parent);
  return inode;
}

int sys_open(const char *filename, int flags, int mode) {
  if (*filename == 0)
    return -ENOENT;

  struct file *file = file_alloc();
  if (file == NULL)
    return -ENOMEM;

  file->flags |= flags;
  file->inode = namei(filename);

  if (file->inode == NULL && flags & O_CREAT) {
    file->inode = creat(filename, mode);
  }

  if (file->inode == NULL) {
    file_close(file);
    return -ENOENT;
  }

  if (flags & O_TRUNC) {
    ilock(file->inode);
    itrunc(file->inode);
    iunlock(file->inode);
  } else if (flags & O_APPEND) {
    file->offset = file->inode->i_size;
  }

  return file_fd_alloc(cur_proc, file);
}

struct file *file_dup(struct file *file) {
  ++file->ref_count;
  return file;
}

int dup(int fd) {
  if (fd < 0 || fd > NFILE || cur_proc->ofile[fd] == NULL)
    return -1;

  struct file *file = cur_proc->ofile[fd];
  int new_fd = file_fd_alloc(cur_proc, file);
  if (new_fd >= 0 && new_fd < NFILE) {
    file_dup(file);
  }

  return new_fd;
}

ssize_t file_read(struct file *file, void *buf, size_t nbytes) {
  int lockstat = ilock(file->inode);
  if (lockstat != 0)
    return lockstat;

  ssize_t ret = readi(file->inode, buf, file->offset, nbytes);
  if (ret > 0) {
    file->offset += ret;
  }
  iunlock(file->inode);

  return ret;
}

ssize_t file_write(struct file *file, const void *buf, size_t nbytes) {
  ilock(file->inode);
  ssize_t ret = writei(file->inode, buf, file->offset, nbytes);
  if (ret > 0) {
    file->offset += ret;
  }
  iunlock(file->inode);

  return ret;
}

int stat(const char *path, struct stat *st) {
  struct inode *inode = namei(path);
  if (inode == NULL)
    return -1;

  ilock(inode);
  istat(inode, st);
  iunlockput(inode);
  return 0;
}

int fstat(int fd, struct stat *st) {
  if (fd < 0 || fd > NFILE || cur_proc->ofile[fd] == NULL)
    return -1;

  struct inode *inode = cur_proc->ofile[fd]->inode;
  ilock(inode);
  istat(inode, st);
  iunlock(inode);
  return 0;
}

int ioctl(int fd, int op, void *ptr) {
  if (fd < 0 || fd >= NFILE || file[fd].ref_count < 1 || file[fd].inode == NULL)
    return -ENOTTY;

  struct inode *inode = file[fd].inode;
  dev_t dev = inode->dev;

  if ((inode->i_mode & S_IFMT) == S_IFCHR)
    dev = inode->i_direct[0];

  if (major(dev) < 0 || major(dev) >= num_cdev || cdevsw[major(dev)].tty == NULL)
    return -ENOTTY;

  return cdevsw[major(dev)].ioctl(dev, op, ptr);
}

long file_seek(struct file *f, long offset, int whence) {
  long new_offset = 0;
  switch (whence) {
  case SEEK_SET:
    new_offset = offset;
    break;
  case SEEK_CUR:
    new_offset = f->offset + offset;
    break;
  case SEEK_END:
    new_offset = f->inode->i_size + offset;
    break;
  default:
    return -EINVAL;
  }

  // Check offset bounds for regular files
  if (new_offset >= f->inode->i_size && (f->inode->i_mode & S_IFMT) == S_IFREG)
    return -EINVAL;

  f->offset = new_offset;
  return f->offset;
}

long lseek(int fd, long offset, int whence) {
  if (fd < 0 || fd >= NFILE || cur_proc->ofile[fd] == NULL)
    return -EBADF;

  return file_seek(cur_proc->ofile[fd], offset, whence);
}