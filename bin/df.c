#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/buf.h>
#include <sys/fs.h>
#include <unistd.h>

#define ROOTDEV "/dev/wd0p1"

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;

  int fd = open(ROOTDEV, O_RDONLY);

  if (fd == -1) {
    fprintf(stderr, "Can't open %s\n", ROOT_DEV);
    exit(EXIT_FAILURE);
  }

  struct fs sb;
  if (read(fd, &sb, sizeof(sb)) != sizeof(sb)) {
    fprintf(stderr, "Read error %s\n", ROOT_DEV);
    exit(EXIT_FAILURE);
  }

  if (sb.magic != MINIX_FS_MAGIC) {
    fprintf(stderr, "Superblock bad magic: %x\n", sb.magic);
    exit(EXIT_FAILURE);
  }

  ssize_t zmap_size = BLOCK_SIZE * sb.zmap_blocks;
  uint8_t *zmap = malloc(zmap_size);
  lseek(fd, BLOCK_SIZE * (1 + sb.imap_blocks), SEEK_SET);
  if (read(fd, zmap, zmap_size) != zmap_size) {
    fprintf(stderr, "lseek errno %d\n", errno);
    exit(EXIT_FAILURE);
  }
  int nzones = sb.nzones;
  uint8_t *zptr = zmap;
  int zalloc = 0;
  while (nzones > 0) {
    int bits = *zptr;
    while (bits > 0) {
      zalloc += bits & 1;

      bits >>= 1;
    }
    nzones -= 8;
    ++zptr;
  }
  long total = sb.nzones * 1024;
  long alloced = zalloc * 1024;
  long usage = alloced * 100 / total;
  long total_mb = total / (1024 * 1024);
  long alloced_mb = alloced / (1024 * 1024);

  printf("Filesystem\tSize\tUsed\tAvail\tUsage\tMountpoint\n");
  printf("%s\t%dMB\t%dMB\t%dMB\t%d%\t/\n", ROOTDEV, total_mb, alloced_mb,
         (total_mb - alloced_mb), usage);

  close(fd);
  return 0;
}