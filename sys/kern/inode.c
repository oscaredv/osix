#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/fs.h>
#include <sys/inode.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/stat.h>
#include <sys/system.h>
#include <time.h>

#define NUM_INODES 32

struct inode inodes[NUM_INODES];

void inode_init() {
  for (int i = 0; i < NUM_INODES; i++) {
    struct inode *inode = &inodes[i];
    inode->dev = -1;
    inode->i_flags = 0;
    inode->i_refcount = 0;
  }
}

unsigned int inode_block(unsigned int inodeno) {
  return FIRST_BLK + super.imap_blocks + super.zmap_blocks + (inodeno - 1) / (BLOCK_SIZE / sizeof(struct minix_inode));
}

unsigned int inode_offset(unsigned int inodeno) { return (inodeno - 1) % (BLOCK_SIZE / sizeof(struct minix_inode)); }

struct inode *iget(dev_t dev, unsigned int inodeno) {
  // Is inode already in cache?
  int i = 0;
  while (i < NUM_INODES) {
    if (inodes[i].dev == dev && inodes[i].i_inodeno == inodeno) {
      struct inode *inode = &inodes[i];

      if (inode->i_flags & I_LOCK) {
        inode->i_flags |= I_WANTED;
        sleep(inode);
        i = 0; // Restart loop
      } else {
        inode->i_flags |= I_LOCK;
        ++inode->i_refcount;

        iunlock(inode);
        return inode;
      }
    } else {
      ++i;
    }
  }

  struct inode *inode = NULL;
  // Find first unused entry
  // TODO: inode free list
  for (int i = 0; i < NUM_INODES; i++) {
    if (inodes[i].i_refcount == 0 && (inodes[i].i_flags & I_DIRTY) == 0) {
      inode = &inodes[i];
      break;
    }
  }

  if (inode == NULL)
    panic("inode cache full!");

  // Reserve and lock inode before reading from disk, because I/O will lead to context switch
  inode->dev = dev;
  inode->i_inodeno = inodeno;
  inode->i_refcount = 1;
  inode->i_flags = I_LOCK;

  // Load inode from disk
  // TODO: check if inode_index is large, and we point outside of buf
  unsigned int block_no = inode_block(inode->i_inodeno);
  unsigned int inode_index = inode_offset(inode->i_inodeno);
  struct buf *buf = bread(dev, block_no);
  struct minix_inode *dinode = (struct minix_inode *)buf->data + inode_index;

  // Copy from disk inode
  inode->i_mode = dinode->di_mode;
  inode->i_nlinks = dinode->di_nlinks;
  inode->i_uid = dinode->di_uid;
  inode->i_gid = dinode->di_gid;
  inode->i_time = dinode->di_time;
  inode->i_size = dinode->di_size;
  for (int b = 0; b < NDIRECT; b++) {
    inode->i_direct[b] = dinode->di_zone[b];
  }
  inode->i_indirect = dinode->di_indirect;
  inode->i_dblindirect = dinode->di_dblindirect;

  brelse(buf);
  iunlock(inode);
  return inode;
}

unsigned int bmap(struct inode *inode, unsigned int index, int flags) {
  if ((inode->i_flags & I_LOCK) == 0)
    panic("bmap: inode not locked");

  unsigned int block_no = 0;
  if (index < NDIRECT) {
    // Direct zone
    if (flags & B_WRITE && inode->i_direct[index] == 0) {
      inode->i_direct[index] = balloc(inode->dev);
      inode->i_flags |= I_DIRTY;
    }

    block_no = inode->i_direct[index];
  } else if (index < (NDIRECT + NINDIRECT)) {
    // Indirect zone
    if (flags & B_WRITE && inode->i_indirect == 0) {
      inode->i_indirect = balloc(inode->dev);
      inode->i_flags |= I_DIRTY;
    }

    if (inode->i_indirect) {
      struct buf *indirect = bread(inode->dev, inode->i_indirect);
      uint16_t *blocks = indirect->data;
      if (blocks[index - NDIRECT] == 0) {
        blocks[index - NDIRECT] = balloc(inode->dev);
        bwrite(indirect);
      }
      block_no = blocks[index - NDIRECT];
      brelse(indirect);
    }
  } else {
    panic("Double indirect not implemented!");
  }

  return block_no;
}

ssize_t readi(struct inode *inode, void *dst, long offset, size_t nbytes) {
  if ((inode->i_flags & I_LOCK) == 0)
    panic("readi not locked");

  if ((inode->i_mode & S_IFMT) == S_IFCHR) {
    dev_t cdev = inode->i_direct[0];
    if (major(cdev) >= num_cdev || cdevsw[major(cdev)].read == NULL) {
      return -ENODEV;
    }
    return cdevsw[major(cdev)].read(inode, cdev, dst, nbytes);
  }

  if ((inode->i_mode & S_IFMT) == S_IFBLK) {
    dev_t bdev = inode->i_direct[0];
    unsigned int block_no = (offset / BLOCK_SIZE) + 1;
    offset = offset % BLOCK_SIZE;

    int len = 0;
    while (nbytes > 0) {
      struct buf *block = bread(bdev, block_no);
      size_t seg_size = BLOCK_SIZE - offset;
      if (seg_size > nbytes)
        seg_size = nbytes;

      memcpy(dst, block->data + offset, seg_size);

      offset = 0;
      len += seg_size;
      nbytes -= seg_size;
      dst += seg_size;
      ++block_no;
      brelse(block);
    }

    return len;
  }

  if (offset > inode->i_size)
    return -EINVAL;

  // TODO: Check error flag from bread()
  long end = offset + nbytes;
  if (end > inode->i_size)
    nbytes = inode->i_size - offset;

  unsigned int bindex = offset / BLOCK_SIZE;
  offset = offset % BLOCK_SIZE;
  long remaining = nbytes;

  while (remaining > 0) {
    unsigned int block_no = bmap(inode, bindex, B_READ);
    unsigned int seg_size = MIN(BLOCK_SIZE - offset, remaining);

    struct buf *buf = bread(inode->dev, block_no);
    memcpy(dst, buf->data + offset, seg_size);

    offset = 0;
    dst += seg_size;
    remaining -= seg_size;
    ++bindex;
    brelse(buf);
  }

  return nbytes;
}

ssize_t writei(struct inode *inode, const void *src, unsigned int offset, size_t nbytes) {
  if ((inode->i_flags & I_LOCK) == 0)
    panic("writei not locked");

  unsigned int orig_offset = offset;
  ssize_t ret = 0;
  if ((inode->i_mode & S_IFMT) == S_IFCHR) {
    dev_t cdev = inode->i_direct[0];
    if (major(cdev) >= num_cdev || cdevsw[major(cdev)].write == NULL) {
      return -ENODEV;
    }
    return cdevsw[major(cdev)].write(cdev, src, nbytes);
  } else if ((inode->i_mode & S_IFMT) == S_IFBLK) {
    panic("writei: block dev\n");
    // } else if ((inode->i_mode & S_IFMT) == S_IFREG) {
    //   panic("writei: regular file\n");
  } else if ((inode->i_mode & S_IFMT) == S_IFIFO) {
    panic("writei: fifo\n");
  } else /*if ((inode->i_mode & S_IFMT) == S_IFDIR)*/ {
    unsigned int bindex = offset / BLOCK_SIZE;
    offset = offset % BLOCK_SIZE;
    size_t remaining = nbytes;

    while (remaining > 0) {
      unsigned int block_no = bmap(inode, bindex, B_WRITE);
      unsigned int seg_size = MIN(BLOCK_SIZE - offset, remaining);

      struct buf *buf = bread(inode->dev, block_no);
      memcpy(buf->data + offset, src, seg_size);
      bwrite(buf);
      brelse(buf);

      offset = 0;
      src += seg_size;
      remaining -= seg_size;
      ret += seg_size;
      ++bindex;
    }

    long size = orig_offset + nbytes;
    if (size > inode->i_size) {
      inode->i_size = size;
      inode->i_flags |= I_DIRTY;
    }
  }

  return ret;
}

int ilock(struct inode *inode) {
  if (inode->i_flags & I_LOCK)
    panic("ilock");

  while (inode->i_flags & I_LOCK) {
    if (signal_pending(cur_proc))
      return -EINTR;

    inode->i_flags |= I_WANTED;
    sleep(inode);

    if (signal_pending(cur_proc))
      return -EINTR;
  }

  inode->i_flags |= I_LOCK;
  return 0;
}

void iunlock(struct inode *inode) {
  if (inode == NULL || (inode->i_flags & I_LOCK) == 0)
    panic("iunlock");

  if (inode->i_flags & I_WANTED) {
    wakeup(inode);
  }
  inode->i_flags &= ~(I_WANTED | I_LOCK);
}

void iunlockput(struct inode *inode) {
  iunlock(inode);
  iput(inode);
}

void iupdate(struct inode *inode) {
  if (!(inode->i_flags & I_DIRTY))
    return;

  unsigned int block_no = inode_block(inode->i_inodeno);
  unsigned int inode_index = inode_offset(inode->i_inodeno);
  struct buf *buf = bread(inode->dev, block_no);
  struct minix_inode *dinode = (struct minix_inode *)buf->data + inode_index;

  dinode->di_mode = inode->i_mode;
  dinode->di_nlinks = inode->i_nlinks;
  dinode->di_uid = inode->i_uid;
  dinode->di_gid = inode->i_gid;
  dinode->di_time = inode->i_time;
  dinode->di_size = inode->i_size;
  for (int b = 0; b < NDIRECT; b++) {
    dinode->di_zone[b] = inode->i_direct[b];
  }
  dinode->di_indirect = inode->i_indirect;
  dinode->di_dblindirect = inode->i_dblindirect;
  inode->i_flags &= ~I_DIRTY;

  bwrite(buf);
  brelse(buf);
}

void itrunc(struct inode *inode) {
  if (inode == NULL)
    panic("itrunc1");
  if (inode == NULL || (inode->i_flags & I_LOCK) == 0)
    panic("itrunc2");

  for (int i = 0; i < NDIRECT; i++) {
    if (inode->i_direct[i]) {
      bfree(inode->dev, inode->i_direct[i]);
      inode->i_direct[i] = 0;
    }
  }

  if (inode->i_indirect) {
    struct buf *buf = bread(inode->dev, inode->i_indirect);
    uint16_t *blocks = (uint16_t *)buf->data;
    for (int i = 0; i < NINDIRECT; i++) {
      if (blocks[i])
        bfree(inode->dev, blocks[i]);
    }
    brelse(buf);
    bfree(inode->dev, inode->i_indirect);
    inode->i_indirect = 0;
  }

  inode->i_size = 0;
  inode->i_flags |= I_DIRTY;
  iupdate(inode);
}

void iput(struct inode *inode) {
  if (inode == NULL || inode->i_refcount == 0 || (inode->i_flags & I_LOCK) != 0)
    panic("iput");

  if (--inode->i_refcount == 0 && inode->i_nlinks == 0) {
    ilock(inode);
    itrunc(inode);
    iunlock(inode);
    ifree(inode->dev, inode->i_inodeno);

    inode->i_flags |= I_DIRTY;
    iupdate(inode);
  }
}

struct inode *idup(struct inode *inode) {
  ++inode->i_refcount;
  return inode;
}

int access(struct inode *inode, int mode) {
  if (inode == NULL)
    return -1;

  // For root or kernel access, allow all permissions
  if (cur_proc == NULL || cur_proc->uid == 0)
    return 0;

  if (inode->i_uid == cur_proc->uid && (inode->i_mode & mode << 6)) // Owner
    return 0;
  if (inode->i_gid == cur_proc->gid && (inode->i_mode & mode << 3)) // Group
    return 0;
  if (inode->i_mode & mode) // Others
    return 0;

  return -1;
}

int nameicore(const char *path, char *name, struct inode **inodep, char parent) {
  const char *p = path;

  if (*p == '/') {
    *inodep = iget(ROOT_DEV, ROOT_INODE);
    while (*p == '/')
      p++;
  } else {
    *inodep = idup(cur_proc->cwd);
  }

  while (*p != 0) {
    const char *e = p;
    while (*e != '/' && *e != 0)
      e++;

    ilock(*inodep);
    // TODO: Check if inode is dir!

    if (access(*inodep, IEXEC)) {
      iunlockput(*inodep);
      return -EACCES;
    }

    unsigned int ndirent = (*inodep)->i_size / sizeof(struct dirent);
    struct dirent dir[ndirent];
    // TODO: check return length
    if (readi(*inodep, &dir, 0, (*inodep)->i_size) != (*inodep)->i_size) {
      iunlockput(*inodep);
      return -EIO;
    }
    iunlock(*inodep);

    struct inode *next_inode = NULL;
    for (unsigned d = 0; d < ndirent; d++) {
      int len = strlen(dir[d].d_name);
      if (dir[d].d_ino != 0 && len == (e - p) && strncmp(dir[d].d_name, p, len) == 0) {
        next_inode = iget((*inodep)->dev, dir[d].d_ino);
        break;
      }
    }

    if (name != NULL) {
      strncpy(name, p, e - p);
      name[e - p] = 0;
    }

    if (parent) {
      p = e;
      while (*p == '/')
        p++;
      if (*p == 0) {
        if (next_inode)
          iput(next_inode);

        return 0;
      }
    }

    if (next_inode == NULL) {
      iput(*inodep);
      return -ENOENT;
    }

    iput(*inodep);
    *inodep = next_inode;

    p = e;
    while (*p == '/')
      p++;
  }

  return 0;
}

int namei(const char *path, struct inode **inodep) { return nameicore(path, NULL, inodep, 0); }
int nameiname(const char *path, char *name, struct inode **inodep) { return nameicore(path, name, inodep, 0); }
int parenti(const char *path, char *name, struct inode **inodep) { return nameicore(path, name, inodep, 1); }

int chdir(const char *path) {
  if (path == NULL)
    return -EINVAL;

  struct inode *cwd = NULL;
  int error = namei(path, &cwd);
  if (error != 0)
    return error;

  if (access(cwd, IEXEC)) {
    iput(cwd);
    return -EACCES;
  }

  if ((cwd->i_mode & S_IFMT) != S_IFDIR) {
    iput(cwd);
    return -ENOTDIR;
  }

  if (cur_proc->cwd != NULL)
    iput(cur_proc->cwd);

  cur_proc->cwd = cwd;
  return 0;
}

void istat(struct inode *inode, struct stat *st) {
  st->st_dev = inode->dev;
  st->st_ino = inode->i_inodeno;
  st->st_mode = inode->i_mode;
  st->st_nlink = inode->i_nlinks;
  st->st_uid = inode->i_uid;
  st->st_gid = inode->i_gid;
  st->st_time = inode->i_time;
  st->st_size = inode->i_size;
  for (int b = 0; b < NDIRECT; b++)
    st->st_direct[b] = inode->i_direct[b];
  st->st_indirect = inode->i_indirect;
  st->st_dblindirect = inode->i_dblindirect;
}

struct inode *ialloc(int mode) {
  unsigned int imap_first_block = 2;
  unsigned int inode_no = 1;
  for (unsigned int imap_block_index = 0; imap_block_index < super.imap_blocks; imap_block_index++) {
    struct buf *imap_buf = bread(ROOT_DEV, imap_block_index + imap_first_block);
    uint8_t *imap = imap_buf->data;

    unsigned int index = 0;
    unsigned int ibit = 0;
    while (ibit == 0 && index < BLOCK_SIZE) {
      if (imap[index] != 0xFF) {
        unsigned int bits = imap[index];
        for (unsigned int b = 0; b < 8; b++) {
          if ((bits & (1 << b)) == 0) {
            ibit = b;
            imap[index] |= 1 << ibit;
            imap_buf->flags |= B_DIRTY;
            brelse(imap_buf);

            struct inode *inode = iget(ROOT_DEV, inode_no);
            if (!inode)
              panic("ialloc");

            inode->i_mode = mode | S_IFREG;
            inode->i_time = time(NULL);
            inode->i_nlinks = 0;
            inode->i_size = 0;
            inode->i_uid = cur_proc->uid;
            inode->i_gid = cur_proc->gid;
            memset(&inode->i_direct, 0, sizeof(inode->i_direct));
            memset(&inode->i_indirect, 0, sizeof(inode->i_indirect));
            memset(&inode->i_dblindirect, 0, sizeof(inode->i_dblindirect));
            inode->i_flags |= I_DIRTY;
            return inode;
          }
          ++inode_no;
        }
      } else {
        inode_no += 8;
      }
      ++index;
    }
    brelse(imap_buf);
  }

  return NULL;
}

void ifree(dev_t dev, unsigned int inodeno) {
  if (inodeno < 1)
    panic("ifree: bad inodeno");

  unsigned int imap_begin = 2; // + super.imap_blocks;
  unsigned imap_block = (inodeno - 1) / (BLOCK_SIZE * 8);
  if (imap_block >= super.imap_blocks)
    panic("ifree: inodeno out of range");

  imap_block += imap_begin;
  unsigned int index = ((inodeno - 1) % (BLOCK_SIZE * 8)) / 8;
  unsigned int bit = (inodeno - 1) % 8;

  struct buf *buf = bread(dev, imap_block);
  uint8_t *imap = buf->data;

  if ((imap[index] & (1 << bit)) == 0)
    panic("bfree: not allocated");

  imap[index] &= ~(1 << bit);
  bwrite(buf);
  brelse(buf);
}
