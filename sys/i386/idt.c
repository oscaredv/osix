#include <i386/idt.h>
#include <i386/pic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/system.h>

#define IRQ0 0x20 // Programmable Interrupt Timer Interrupt
#define IRQ1 0x21 // Keyboard Interrupt
#define IRQ2 0x22
#define IRQ3 0x23 // COM2
#define IRQ4 0x24 // COM1
#define IRQ5 0x25 // LPT2
#define IRQ6 0x26 // Floppy Disk
#define IRQ7 0x27 // LPT1
#define IRQ8 0x28 // CMOS real-time clock
#define IRQ9 0x29
#define IRQ10 0x2a
#define IRQ11 0x2b
#define IRQ12 0x2c // PS2 Mouse
#define IRQ13 0x2d // FPU
#define IRQ14 0x2e // Primary ATA Hard Disk
#define IRQ15 0x2f // Secondary ATA Hard Disk

#define INTERRUPT_GATE 0x8e
#define TRAP_GATE 0x8f
#define DPL 0x60

typedef struct {
  uint16_t base_low;
  uint16_t segment;
  uint8_t reserved;
  uint8_t flags;
  uint16_t base_high;
} __attribute__((packed)) idt_descr_t;

typedef struct {
  uint16_t limit;
  uint32_t offset;
} __attribute__((packed)) idtr_t;

idt_descr_t idt_descr[256];
idtr_t idtr;

void idt_set_descr(uint8_t n, void *isr, uint8_t flags) {
  idt_descr[n].base_low = (uint32_t)isr & 0xFFFF;
  idt_descr[n].base_high = (uint32_t)isr >> 16;
  idt_descr[n].flags = flags;
  idt_descr[n].segment = 0x08;
  idt_descr[n].reserved = 0;
}

const char *trap_name[] = {"DIVISION BY ZERO",
                           "DEBUG",
                           "NMI",
                           "BREAKPOINT",
                           "OVERFLOW EXCEPTION",
                           "BOUND RANGE EXCEEDED",
                           "INVALID OPCODE",
                           "NO FPU",
                           "DOUBLE FAULT",
                           "",
                           "INVALID TSS",
                           "SEGMENT NOT PRESENT",
                           "STACK SEGMENT FAULT",
                           "GENERAL PROTECTION FAULT",
                           "PAGE FAULT EXCEPTION",
                           "",
                           "FLOATING POINT EXCEPTION",
                           "ALIGNMENT CHECK EXCEPTION",
                           "MACHINE CHECK EXCEPTION",
                           "SIMD FLOATING POINT EXCEPTION"};

void *irq_handler[16];

void isr_handler(struct interrupt_frame *r) {
  if (r->int_no >= IRQ0 && r->int_no <= IRQ15) {
    uint8_t irq = r->int_no - IRQ0;
    if (irq_handler[irq] != NULL) {
      // Call IRQ handler
      ((void (*)())irq_handler[irq])();
    } else {
      printf("Unknown irq: %d\n", irq);
      asm("hlt");
    }
  } else if (r->int_no < IRQ0) {
    printf("Trap: %d", r->int_no);
    if (r->int_no <= 19) {
      printf(", %s", trap_name[r->int_no]);
    }
    printf(", error: %d\n", r->err_code);
    printf("EAX=%08x EBX=%08x ECX=%08x EDX=%08x\n", r->eax, r->ebx, r->ecx, r->edx);
    printf("ESP=%08x EBP=%08x ESI=%08x EDI=%08x\n", r->esp, r->ebp, r->esi, r->edi);
    printf("DS=%04x ES=%04x FS=%04x GS=%04x\n", r->ds, r->ds, r->ds, r->ds);
    printf("EIP=%08x CS=%04x EFLAGS=%08x usr=%x SS=%04x\n", r->eip, r->cs, r->eflags, r->user_esp, r->ss);

    if ((r->cs & 3) == 3) // User process crashed
      exit(-1);

    // Kernel crashed
    asm("hlt");
  } else {
    printf("Unknown interrupt: %d\n", r->int_no);
    asm("hlt");
  }
}

extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();

extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();

extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();

extern void isr30();
extern void isr31();
extern void isr32();
extern void isr33();
extern void isr34();
extern void isr35();
extern void isr36();
extern void isr37();
extern void isr38();
extern void isr39();

extern void isr40();
extern void isr41();
extern void isr42();
extern void isr43();
extern void isr44();
extern void isr45();
extern void isr46();
extern void isr47();

extern void isr128();

void idt_init(void) {
  memset(irq_handler, 0, sizeof(irq_handler));

  idtr.limit = sizeof(idt_descr) - 1;
  idtr.offset = (uint32_t)idt_descr;
  memset(&idt_descr, 0, sizeof(idt_descr));

  idt_set_descr(0, isr0, INTERRUPT_GATE);
  idt_set_descr(1, isr1, INTERRUPT_GATE);
  idt_set_descr(2, isr2, INTERRUPT_GATE);
  idt_set_descr(3, isr3, INTERRUPT_GATE);
  idt_set_descr(4, isr4, INTERRUPT_GATE);
  idt_set_descr(5, isr5, INTERRUPT_GATE);
  idt_set_descr(6, isr6, INTERRUPT_GATE);
  idt_set_descr(7, isr7, INTERRUPT_GATE);
  idt_set_descr(8, isr8, INTERRUPT_GATE);
  idt_set_descr(9, isr9, INTERRUPT_GATE);

  idt_set_descr(10, isr10, INTERRUPT_GATE);
  idt_set_descr(11, isr11, INTERRUPT_GATE);
  idt_set_descr(12, isr12, INTERRUPT_GATE);
  idt_set_descr(13, isr13, INTERRUPT_GATE);
  idt_set_descr(14, isr14, INTERRUPT_GATE);
  idt_set_descr(15, isr15, INTERRUPT_GATE);
  idt_set_descr(16, isr16, INTERRUPT_GATE);
  idt_set_descr(17, isr17, INTERRUPT_GATE);
  idt_set_descr(18, isr18, INTERRUPT_GATE);
  idt_set_descr(19, isr19, INTERRUPT_GATE);

  idt_set_descr(20, isr20, INTERRUPT_GATE);
  idt_set_descr(21, isr21, INTERRUPT_GATE);
  idt_set_descr(22, isr22, INTERRUPT_GATE);
  idt_set_descr(23, isr23, INTERRUPT_GATE);
  idt_set_descr(24, isr24, INTERRUPT_GATE);
  idt_set_descr(25, isr25, INTERRUPT_GATE);
  idt_set_descr(26, isr26, INTERRUPT_GATE);
  idt_set_descr(27, isr27, INTERRUPT_GATE);
  idt_set_descr(28, isr28, INTERRUPT_GATE);
  idt_set_descr(29, isr29, INTERRUPT_GATE);

  idt_set_descr(30, isr30, INTERRUPT_GATE);
  idt_set_descr(31, isr31, INTERRUPT_GATE);
  idt_set_descr(IRQ0, isr32, INTERRUPT_GATE);
  idt_set_descr(IRQ1, isr33, INTERRUPT_GATE);
  idt_set_descr(IRQ2, isr34, INTERRUPT_GATE);
  idt_set_descr(IRQ3, isr35, INTERRUPT_GATE);
  idt_set_descr(IRQ4, isr36, INTERRUPT_GATE);
  idt_set_descr(IRQ5, isr37, INTERRUPT_GATE);
  idt_set_descr(IRQ6, isr38, INTERRUPT_GATE);
  idt_set_descr(IRQ7, isr39, INTERRUPT_GATE);

  idt_set_descr(IRQ8, isr40, INTERRUPT_GATE);
  idt_set_descr(IRQ9, isr41, INTERRUPT_GATE);
  idt_set_descr(IRQ10, isr42, INTERRUPT_GATE);
  idt_set_descr(IRQ11, isr43, INTERRUPT_GATE);
  idt_set_descr(IRQ12, isr44, INTERRUPT_GATE);
  idt_set_descr(IRQ13, isr45, INTERRUPT_GATE);
  idt_set_descr(IRQ14, isr46, INTERRUPT_GATE);
  idt_set_descr(IRQ15, isr47, INTERRUPT_GATE);

  idt_set_descr(0x80, isr128, INTERRUPT_GATE | DPL);

  asm("lidt (idtr)");
}

void idt_set_isr(unsigned char irq, void *isr) {
  if (irq <= IRQ15) {
    irq_handler[irq] = isr;
  }
}
