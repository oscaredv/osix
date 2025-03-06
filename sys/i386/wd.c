#include <i386/idt.h>
#include <i386/io.h>
#include <i386/pic.h>
#include <i386/wd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/param.h>
#include <sys/proc.h>

typedef struct {
  uint8_t part_type;
  uint32_t offset;
  uint32_t size;
} part_t;

typedef struct {
  uint8_t drive;
  uint16_t port;
  uint16_t control_port;
  uint32_t blocks;
  struct buf *buf;
  part_t part[4];
} wd_t;

wd_t wd_devsw[IDE_MAX_DEV];

void wd_intr(wd_t *wd) {
  uint8_t status = inb(IDE_REG_STAT(wd->port));
  // printf("wd_irq status=0x%x", status);

  // if (status & IDE_STAT_ERR)
  //   printf(" ERR");
  // if (status & IDE_STAT_DRQ)
  //   printf(" DRQ");
  // if (status & IDE_STAT_RDY)
  //   printf(" RDY");
  // if (status & IDE_STAT_BSY)
  //   printf(" BSY");
  //  printf("\n");

  // Data ready?
  if (status & IDE_STAT_DRQ && wd->buf != NULL) {
    uint16_t *dst = wd->buf->data;
    for (int i = 0; i < BLOCK_SIZE / 2; i++) {
      // wd->buf->d.u16[i] = inw(IDE_REG_DATA(wd->port));
      dst[i] = inw(IDE_REG_DATA(wd->port));
    }
    iodone(wd->buf);

    wd->buf = NULL;
  }
  wakeup(wd_intr);
}

void wd_intr0() {
  uint8_t drive = inb(IDE_REG_DRIVE(IDE_PORT_PRIMARY));
  if (drive == IDE_MASTER) {
    wd_intr(&wd_devsw[0]);
  } else if (drive == IDE_SLAVE) {
    wd_intr(&wd_devsw[1]);
  }
  pic2_ack();
}

void wd_intr1() {
  uint8_t drive = inb(IDE_REG_DRIVE(IDE_PORT_SECONDARY));
  if (drive == IDE_MASTER) {
    wd_intr(&wd_devsw[2]);
  } else if (drive == IDE_SLAVE) {
    wd_intr(&wd_devsw[3]);
  }
  pic2_ack();
}

void wd_probe(unsigned dev_no, uint16_t port, uint16_t control_port, uint8_t drive, int irq, void *isr) {
  (void)irq;
  (void)isr;
  wd_t *dev = &wd_devsw[dev_no];
  memset(dev, 0, sizeof(wd_t));
  dev->drive = drive;
  dev->port = port;
  dev->control_port = control_port;

  // Select drive
  outb(drive, IDE_REG_DRIVE(port));

  // wait 400ns for drive select to take effect
  inb(control_port);
  inb(control_port);
  inb(control_port);
  inb(control_port);

  // Clear sectorcount & LBA
  outb(0x00, IDE_REG_SECTOR_COUNT(port));
  outb(0x00, IDE_REG_LBA_LO(port));
  outb(0x00, IDE_REG_LBA_MID(port));
  outb(0x00, IDE_REG_LBA_HI(port));

  // IDENTIFY command
  outb(0xEC, IDE_REG_CMD(port));

  // Disk present?
  uint8_t status = inb(IDE_REG_STAT(port));
  if (status != 0 && (status & IDE_STAT_ERR) == 0) {

    while (inb(IDE_REG_STAT(port)) & IDE_STAT_BSY)
      ;

    printf("wd%d io=0x%x irq=%d status=0x%x ", dev_no, port, irq, status);
    if (inb(IDE_REG_STAT(port)) & IDE_STAT_DRQ) {
      uint16_t data[SECTOR_SIZE / 2];
      for (int i = 0; i < 256; i++) {
        data[i] = inw(IDE_REG_DATA(port));
      }

      printf("DMA=%d:%d ", data[88] & 255, data[88] >> 8);

      dev->blocks = data[61] << 16 | data[60];
      printf("%d blocks (%d MB)\n", dev->blocks, (dev->blocks * SECTOR_SIZE) >> 20);
    }

    idt_set_isr(irq, isr);
  }
}

void wd_init(void) {
  wd_probe(0, IDE_PORT_PRIMARY, IDE_CONTROL_PORT_PRIMARY, IDE_MASTER, 14, wd_intr0);
  wd_probe(1, IDE_PORT_PRIMARY, IDE_CONTROL_PORT_PRIMARY, IDE_SLAVE, 14, wd_intr0);
  wd_probe(2, IDE_PORT_SECONDARY, IDE_CONTROL_PORT_SECONDARY, IDE_MASTER, 15, wd_intr1);
  wd_probe(3, IDE_PORT_SECONDARY, IDE_CONTROL_PORT_SECONDARY, IDE_SLAVE, 15, wd_intr1);

  if (wd_devsw[0].blocks > 0 || wd_devsw[1].blocks > 0) {
    pic_enable_interrupt(14);
  }
  if (wd_devsw[2].blocks > 0 || wd_devsw[3].blocks > 0) {
    pic_enable_interrupt(15);
  }

  for (int d = 0; d < 4; d++) {
    if (wd_devsw[d].blocks == 0)
      continue;

    struct buf *buf = bread(makedev(0, 0), 0);
    struct mbr *mbr = buf->data;
    // TODO: Check buffer status
    if (mbr->signature == MBR_SIGNATURE) {
      for (int p = 0; p < 4; p++) {
        if (mbr->part[p].ptype != 0) {
          wd_devsw[d].part[d].part_type = mbr->part[p].ptype;
          wd_devsw[d].part[d].offset = mbr->part[p].lba_start;
          wd_devsw[d].part[d].size = mbr->part[p].lba_size;

          printf("wd%dp%d type=0x%x start=%d size=%dMB\n", d, (p + 1), wd_devsw[d].part[p].part_type,
                 wd_devsw[d].part[p].offset, ((wd_devsw[d].part[p].size * SECTOR_SIZE) >> 20));
        }
      }
    }
    brelse(buf);
  }
}

void wd_read(struct buf *buf) {
  unsigned int sector = buf->block_no * 2;

  uint8_t drive_no = minor(buf->dev) >> 4;
  if (drive_no >= IDE_MAX_DEV || wd_devsw[drive_no].blocks == 0) {
    buf->flags |= B_ERROR;
    iodone(buf);
    return;
  }

  wd_t *wd = &wd_devsw[drive_no];
  uint8_t part_no = minor(buf->dev) & 0x0F;
  if (part_no > 0) {
    if (part_no > 4 || wd_devsw[drive_no].part[part_no - 1].part_type == 0) {
      buf->flags |= B_ERROR;
      iodone(buf);
      return;
    }
    part_t *part = &wd->part[part_no - 1];
    sector += part->offset;
  }

  // Busy wait until drive is ready
  while (inb(IDE_REG_STAT(wd->port)) & IDE_STAT_BSY)
    ;

  // Store data in this buffer
  wd->buf = buf;

  outb(2, IDE_REG_SECTOR_COUNT(wd->port));
  outb(sector & 0xFF, IDE_REG_LBA_LO(wd->port));
  outb((sector >> 8) & 0xFF, IDE_REG_LBA_MID(wd->port));
  outb((sector >> 16) & 0xFF, IDE_REG_LBA_HI(wd->port));
  outb(wd->drive | ((sector >> 24) & 0x0F), IDE_REG_DRIVE(wd->port));
  outb(IDE_CMD_READ_MULTIPLE, IDE_REG_CMD(wd->port));

  if (cur_proc != NULL) {
    // Sleep until I/O is done
    while (wd->buf != NULL) {
      sleep(wd_intr);
    }
  } else {
    // If I/O was initiated by the kernel, sleep()/wakeup() will not work - buzy wait instead
    while (wd->buf != NULL) {
      asm("sti");
      asm("hlt");
      asm("cli");
    }
  }
}

void wd_write(struct buf *buf) {
  (void)buf;
  printf("wd_write()\n");
}
