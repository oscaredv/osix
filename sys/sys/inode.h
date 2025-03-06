#ifndef __SYS_INODE_H__
#define __SYS_INODE_H__

#include <sys/types.h>
#include <time.h>

#define I_LOCK 1    // inode is locked
#define I_WANTED 2  // wakeup() must be called on unlock, some other process wants to lock
#define I_UPDATED 4 // inode has been updated

#define NDIRECT 7
#define NINDIRECT 512

struct inode {
  dev_t dev;
  unsigned int i_inodeno;

  unsigned int i_flags;
  unsigned int i_refcount;

  unsigned short i_mode;
  unsigned short i_nlinks;
  unsigned short i_uid;
  unsigned short i_gid;
  time_t i_time;
  long i_size;
  unsigned short i_direct[NDIRECT];
  unsigned short i_indirect;
  unsigned short i_dblindirect;
};

struct stat;

struct inode *iget(dev_t dev, unsigned int inodeno);
unsigned int bmap(struct inode *inode, unsigned int blockno, int flags);
ssize_t readi(struct inode *inode, void *dst, long offset, size_t nbytes);
ssize_t writei(struct inode *inode, const void *src, unsigned int offset, size_t nbytes);
struct inode *namei(const char *path);
struct inode *parenti(const char *path, char *name);
struct inode *idup(struct inode *inode);
int ilock(struct inode *inode);
void iunlock(struct inode *inode);
void iunlockput(struct inode *inode);
void iput(struct inode *inode);
void istat(struct inode *inode, struct stat *st);
void itrunc(struct inode *inode);
struct inode *ialloc(int mode);
void ifree(dev_t dev, unsigned int inodeno);

#endif
