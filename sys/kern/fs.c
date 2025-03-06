#include <stdio.h>
#include <string.h>
#include <sys/buf.h>
#include <sys/fs.h>
#include <sys/inode.h>

struct fs super;
struct inode *root_inode = NULL;

void readsb(dev_t dev) {
  struct buf *buf = bread(dev, 1);
  memcpy(&super, buf->data, sizeof(struct fs));
  brelse(buf);
}

void mount(dev_t dev) {
  readsb(dev);
  if (super.magic != MINIX_FS_MAGIC) {
    printf("mount: Device %d:%d: Not a minix filesystem\n", major(dev), minor(dev));
    return;
  }
  if ((super.state & FS_VALID) == 0) {
    printf("mount: Device %d:%d: Filesystem not valid\n", major(dev), minor(dev));
    return;
  }
  if ((super.state & FS_ERROR) != 0) {
    printf("mount: Device %d:%d: Filesystem has errors\n", major(dev), minor(dev));
    return;
  }

  root_inode = iget(dev, ROOT_INODE);
}
