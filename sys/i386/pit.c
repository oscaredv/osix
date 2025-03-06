#include <i386/cons.h>
#include <i386/idt.h>
#include <i386/pic.h>
#include <i386/pit.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/proc.h>
#include <sys/system.h>

#define PIT_COMMAND 0x43
#define PIT_DATA0 0x40
#define PIT_DATA1 0x41
#define PIT_DATA2 0x42

unsigned int pit_ticks = 0;
unsigned int pit_ticks_loop = 0;

// Time in seconds
time_t pit_time = 0;

void pit_irq() {
  ++pit_ticks;

  // Wake up sleeping processes when sleep time is done
  wakeup((void *)pit_ticks);

  if (++pit_ticks_loop >= PIT_FREQ) {
    pit_ticks_loop = 0;
    ++pit_time;
  }

  pic_ack();

  // Preempt running process
  yield();
}

void pit_settime(time_t time) { pit_time = time; }

time_t pit_gettime(void) { return pit_time; }

void pit_init(void) {
  idt_set_isr(0, &pit_irq);

  // Write divisor to PIT
  outb(0x36, PIT_COMMAND);
  unsigned divider = 1193180 / PIT_FREQ;
  outb((uint8_t)divider, PIT_DATA0);
  outb((uint8_t)(divider >> 8), PIT_DATA0);

  printf("pit0 at I/O 0x%x, %d Hz\n", PIT_DATA0, PIT_FREQ);
  pic_enable_interrupt(0);
}
