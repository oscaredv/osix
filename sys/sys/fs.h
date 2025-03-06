#ifndef __SYS_FS_H__
#define __SYS_FS_H__

#include <stdint.h>
#include <sys/types.h>

#define ROOT_DEV makedev(0, 1)
#define ROOT_INODE 1

#define MINIX_FS_MAGIC 0x138F
#define FS_VALID 0x0001
#define FS_ERROR 0x0002

// Filesystem superblock
struct fs {
  uint16_t ninodes;
  uint16_t nzones;
  uint16_t imap_blocks;
  uint16_t zmap_blocks;
  uint16_t firstdatazone;
  uint16_t log_zone_size;
  uint32_t max_size;
  uint16_t magic;
  uint16_t state;
  uint32_t zones;
};

extern struct fs super;

#define MINIX_NDIRECT 7

struct minix_inode {
  uint16_t di_mode;
  uint16_t di_uid;
  uint32_t di_size;
  uint32_t di_time;
  uint8_t di_gid;
  uint8_t di_nlinks;
  uint16_t di_zone[MINIX_NDIRECT];
  uint16_t di_indirect;
  uint16_t di_dblindirect;
};

void mount(dev_t dev);

#endif
