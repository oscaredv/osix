#ifndef __SYS_BUF_H__
#define __SYS_BUF_H__

#include <stdint.h>
#include <sys/types.h>

#define BLOCK_SIZE 1024
#define NBUF 64

#define B_READ 1    // Will be read from disk
#define B_WRITE 2   // Will write to disk
#define B_DONE 4    // I/O operation finished
#define B_ERROR 8   // I/O failed
#define B_BUSY 16   // Buffer is in use
#define B_WANTED 32 // wakeup() must be called to wake up sleeping processes that want this buffer
#define B_DIRTY 64  // Buffer is dirty and should be written to disk

struct buf {
  struct buf *next;
  struct buf *prev;
  dev_t dev;
  unsigned int block_no;
  unsigned int flags;
  unsigned int ref_count;
  void *data;
};

struct buf *bread(dev_t dev, unsigned int block_no);
void bwrite(struct buf *buf);
void brelse(struct buf *buf);
void iodone(struct buf *buf);
unsigned int balloc(dev_t dev);
void bfree(dev_t dev, unsigned int block_no);

#endif