#ifndef __I386_WD_H__
#define __I386_WD_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#define SECTOR_SIZE 512
#define IDE_MAX_DEV 4

#define IDE_PORT_PRIMARY 0x1F0
#define IDE_PORT_SECONDARY 0x170
#define IDE_CONTROL_PORT_PRIMARY 0x3F6
#define IDE_CONTROL_PORT_SECONDARY 0x376
#define IDE_MASTER 0xE0
#define IDE_SLAVE 0xF0

#define IDE_REG_DATA(base) base + 0         // RW
#define IDE_REG_ERR(base) base + 1          // R
#define IDE_REG_FEAT(base) base + 1         // W
#define IDE_REG_SECTOR_COUNT(base) base + 2 // RW
#define IDE_REG_LBA_LO(base) base + 3       // RW
#define IDE_REG_LBA_MID(base) base + 4      // RW
#define IDE_REG_LBA_HI(base) base + 5       // RW
#define IDE_REG_DRIVE(base) base + 6        // RW
#define IDE_REG_STAT(base) base + 7         // R
#define IDE_REG_CMD(base) base + 7          // W

#define IDE_STAT_ERR 0x01 // Error
#define IDE_STAT_DRQ 0x08 // Data request
#define IDE_STAT_RDY 0x40 // Drive ready
#define IDE_STAT_BSY 0x80 // Drive busy

#define IDE_CMD_READ 0x20
#define IDE_CMD_WRITE 0x30
#define IDE_CMD_READ_MULTIPLE 0xC4
#define IDE_CMD_WRITE_MULTIPLE 0xC5

struct mbr_part {
  uint8_t active;
  uint8_t first_chs[3];
  uint8_t ptype;
  uint8_t last_chs[3];
  uint32_t lba_start;
  uint32_t lba_size;
} __attribute__((packed));

#define MBR_SIGNATURE 0xAA55

struct mbr {
  uint8_t boot_code[446];
  struct mbr_part part[4];
  uint16_t signature;
} __attribute__((packed));

void wd_init(void);

struct buf;
void wd_read(struct buf *buf);
void wd_write(struct buf *buf);

#endif
