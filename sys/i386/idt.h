#ifndef __I386_IDT_H__
#define __I386_IDT_H__

#include <stdint.h>

struct interrupt_frame {
  uint32_t ds;                                     // Data segment selector
  uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
  uint32_t int_no, err_code;                       // Interrupt number and error code (if applicable)
  uint32_t eip, cs, eflags, user_esp, ss;          // Pushed by the processor automatically.
};

void idt_init();

void idt_set_isr(unsigned char irq, void *isr);

#endif
