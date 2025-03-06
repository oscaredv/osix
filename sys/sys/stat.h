#ifndef __SYS_STAT_H__
#define __SYS_STAT_H__

#include <sys/types.h>
#include <time.h>

#define S_IFMT 0xF000  // Inode type mask
#define S_IFREG 0x8000 // Regular file
#define S_IFBLK 0x6000 // Block device
#define S_IFDIR 0x4000 // Directory
#define S_IFCHR 0x2000 // Character device
#define S_IFIFO 0x1000 // FIFO/named pipe

#define S_ISUID 0004000 // Set UID on execute
#define S_ISGID 0002000 // Set GID on execute
#define S_ISVTX 0001000

#define S_IRWXU 00700 // User rwx
#define S_IRUSR 00400 // User read
#define S_IWUSR 00200 // User write
#define S_IXUSR 00100 // User execute

#define S_IRWXG 00070 // Group rwx
#define S_IRGRP 00040 // Group read
#define S_IWGRP 00020 // Group write
#define S_IXGRP 00010 // Group execute

#define S_IRWXO 00007 // Other rwx
#define S_IROTH 00004 // Other read
#define S_IWOTH 00002 // Other write
#define S_IXOTH 00001 // Other execute

struct stat {
  dev_t st_dev;
  unsigned int st_ino;
  unsigned short st_mode;
  unsigned short st_nlink;
  unsigned short st_uid;
  unsigned short st_gid;
  long st_size;
  time_t st_time;

  unsigned short st_direct[7];
  unsigned short st_indirect;
  unsigned short st_dblindirect;
};

int stat(const char *path, struct stat *st);
int fstat(int fd, struct stat *st);

#endif