#include <i386/pic.h>
#include <stdio.h>

void pic_init() {
  // Remap PIC
  outb(ICW1_INIT | ICW1_ICW4, PIC1_CMD_PORT);
  io_wait();
  outb(ICW1_INIT | ICW1_ICW4, PIC2_CMD_PORT);
  io_wait();
  outb(PIC1_OFFSET, PIC1_DATA_PORT);
  printf("pic0 irq offset 0x%x\n", PIC1_OFFSET);
  io_wait();
  outb(PIC2_OFFSET, PIC2_DATA_PORT);
  printf("pic1 irq offset 0x%x\n", PIC2_OFFSET);
  io_wait();
  outb(0x04, PIC1_DATA_PORT);
  io_wait();
  outb(0x02, PIC2_DATA_PORT);
  io_wait();

  outb(ICW4_8086, PIC1_DATA_PORT);
  io_wait();
  outb(ICW4_8086, PIC2_DATA_PORT);
  io_wait();

  // Mask all interrupts
  outb(0xFF, PIC1_DATA_PORT);
  io_wait();
  outb(0xFF, PIC2_DATA_PORT);

  // Enable interrupts cascading from PIC1
  pic_enable_interrupt(2);
}

void pic_enable_interrupt(int irq_no) {
  if (irq_no < 0 || irq_no >= 16)
    return;

  unsigned char port = PIC1_DATA_PORT;
  if (irq_no >= 8) {
    port = PIC2_DATA_PORT;
    irq_no &= 0x07;
  }

  unsigned short mask = inb(port);
  mask ^= 1 << irq_no;
  outb(mask, port);
}
