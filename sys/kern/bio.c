#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/fs.h>
#include <sys/system.h>
#include <sys/types.h>

struct buf bcache[NBUF];
uint8_t bdata[NBUF][BLOCK_SIZE];

struct buf bfreelist; // List of free buffers

struct buf *getblk(dev_t dev, unsigned int block_no) {
  if (major(dev) >= num_bdev)
    panic("getblk");

  int b = 0;
  while (b < NBUF) {
    struct buf *buf = &bcache[b];
    if (buf->dev == dev && buf->block_no == block_no) {
      // Buffer found, is it busy?
      if (buf->flags & B_BUSY) {
        buf->flags |= B_WANTED;
        sleep(buf);

        // Restart the loop because the buffers may have changed while we slept
        b = 0;
      } else {
        if (buf->ref_count == 0) {
          // Remove buffer from free list
          buf->prev->next = buf->next;
          buf->next->prev = buf->prev;
        }
        ++buf->ref_count;
        return buf;
      }
    } else {
      ++b;
    }
  }

  if (bfreelist.next == &bfreelist) {
    // TODO: sleep instead of panic!
    panic("no buffers free");
  }

  struct buf *buf = bfreelist.next;
  // TODO: if dirty, write first

  // Initialize
  buf->dev = dev;
  buf->block_no = block_no;
  buf->flags = B_BUSY;
  buf->ref_count = 1;

  // Remove from free list
  buf->prev->next = buf->next;
  buf->next->prev = buf->prev;

  return buf;
}

struct buf *bread(dev_t dev, unsigned int block_no) {
  struct buf *buf = getblk(dev, block_no);

  if ((buf->flags & B_DONE) == 0) {
    buf->flags |= B_READ;

    bdevsw[major(dev)].read(buf);
  }

  return buf;
}

void bwrite(struct buf *buf) { buf->flags |= B_DIRTY; }

void brelse(struct buf *buf) {
  if (buf->flags & B_WANTED) {
    wakeup(buf);
  }

  buf->flags &= ~(B_WANTED | B_BUSY);

  if (buf->ref_count == 0)
    panic("bad ref_count");

  if (--buf->ref_count == 0 && (buf->flags & B_DIRTY) == 0) {
    // Add to end of free list
    buf->next = &bfreelist;
    buf->prev = bfreelist.prev;
    bfreelist.prev->next = buf;
    bfreelist.prev = buf;
  }
}

void iodone(struct buf *buf) {
  buf->flags |= B_DONE;

  if (buf->flags & B_WANTED) {
    // Wake up process(es) waiting for this buffer
    wakeup(buf);
  }
  buf->flags &= ~(B_WANTED | B_READ);
}

unsigned int balloc(dev_t dev) {
  unsigned int zmap_begin = 2 + super.imap_blocks; // First block is 1, skip superblock and imap blocks
  unsigned int zmap_end = zmap_begin + super.zmap_blocks;
  unsigned int block_no = super.firstdatazone - 1; // Subtract one because block addressing starts at 1

  for (unsigned int zmap_block = zmap_begin; zmap_block < zmap_end; zmap_block++) {
    struct buf *buf = bread(dev, zmap_block);
    uint8_t *zmap = buf->data;

    for (unsigned int z = 0; z < BLOCK_SIZE; z++) {
      if (zmap[z] != 0xFF) { // Skip bitmap bytes in which all blocks are allocated
        unsigned int bits = zmap[z];
        for (unsigned int b = 0; b < 8; b++) {
          if ((bits & (1 << b)) == 0) { // Find first unset bit
            zmap[z] |= 1 << b;          // Mark as allocated
            bwrite(buf);
            brelse(buf);
            return block_no; // Allocated block number
          }
          ++block_no;
        }
      } else {
        block_no += 8;
      }
    }
    brelse(buf);
  }
  return 0; // No free blocks!
}

void bfree(dev_t dev, unsigned int block_no) {
  if (block_no < 1 || block_no > super.nzones)
    panic("bfree: bad block_no");

  unsigned int zone_index = block_no - super.firstdatazone + 1;

  unsigned int zmap_begin = 2 + super.imap_blocks;
  unsigned zmap_block = zone_index / (BLOCK_SIZE * 8);

  zmap_block += zmap_begin;
  unsigned int index = (zone_index % (BLOCK_SIZE * 8)) / 8;
  unsigned int bit = zone_index % 8;

  struct buf *buf = bread(dev, zmap_block);
  uint8_t *zmap = buf->data;

  if ((zmap[index] & (1 << bit)) == 0)
    panic("bfree: not allocated");

  zmap[index] &= ~(1 << bit);
  bwrite(buf);
  brelse(buf);
}

void buf_init() {
  bfreelist.next = bfreelist.prev = &bfreelist;

  for (int b = 0; b < NBUF; b++) {
    struct buf *buf = &bcache[b];

    buf->next = &bfreelist;
    buf->prev = bfreelist.prev;
    buf->prev->next = buf;
    bfreelist.prev = buf;

    buf->flags = 0;
    buf->dev = -1;
    buf->data = bdata[b];
  }
}
