#ifndef _I386_PIC_H_
#define _I386_PIC_H_

#include <i386/io.h>

#define PIC1_CMD_PORT 0x20
#define PIC1_DATA_PORT 0x21
#define PIC2_CMD_PORT 0xA0
#define PIC2_DATA_PORT 0xA1

#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28

#define PIC_ACK 0x20

#define ICW1_ICW4 0x01
#define ICW1_INIT 0x10
#define ICW4_8086 0x01

void pic_init();

static inline void pic_ack() { outb(PIC_ACK, PIC1_CMD_PORT); }

static inline void pic2_ack() {
  outb(PIC_ACK, PIC1_CMD_PORT);
  outb(PIC_ACK, PIC2_CMD_PORT);
}

void pic_enable_interrupt(int irq_no);

#endif