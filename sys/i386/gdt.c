#include <stdint.h>
#include <string.h>
#include <sys/proc.h>

typedef struct {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  unsigned limit_high : 4;
  unsigned flags : 4;
  uint8_t base_high;
} __attribute__((packed)) gdt_descr_t;

typedef struct {
  uint16_t limit;
  uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

gdt_descr_t gdt[6];
gdt_ptr_t gdt_ptr;

// Task state segment (TSS)
// When returning from ring 3 to 0, the esp0 stack pointer for the interrupt is read from here
// The rest of the TSS is irrelevant
typedef struct {
  uint32_t link;
  uint32_t esp0;
  uint32_t ss0;
  uint32_t esp1;
  uint32_t ss1;
  uint32_t esp2;
  uint32_t ss2;
  uint32_t cr3;
  uint32_t eip;
  uint32_t eflags;
  uint32_t eax;
  uint32_t ecx;
  uint32_t edx;
  uint32_t ebx;
  uint32_t esp;
  uint32_t ebp;
  uint32_t esi;
  uint32_t edi;
  uint32_t es;
  uint32_t cs;
  uint32_t ss;
  uint32_t ds;
  uint32_t fs;
  uint32_t gs;
  uint32_t ldt;
  uint16_t trap;
  uint16_t iomap_base;
} __attribute__((packed)) tss_t;

tss_t tss;

void gdt_set_descr(int index, uint32_t base, uint32_t limit, uint8_t access, uint8_t flags) {
  gdt[index].base_low = base & 0xFFFF;
  gdt[index].base_middle = (base >> 16) & 0xFF;
  gdt[index].base_high = (base >> 24) & 0xFF;
  gdt[index].limit_low = limit & 0xFFFF;

  gdt[index].limit_high = (limit >> 16) & 0x0F;
  gdt[index].flags = flags;

  gdt[index].access = access;
}

void tss_set_descr(int32_t index, uint16_t ss0, uint32_t esp0) {
  gdt_set_descr(index, (uint32_t)&tss, sizeof(tss_t), 0xE9, 0x00);

  memset(&tss, 0, sizeof(tss_t));
  tss.ss0 = ss0;
  tss.esp0 = esp0;

  tss.cs = 0x08 | 3;
  tss.ss = tss.ds = tss.es = tss.fs = tss.gs = 0x10 | 3;
}

void tss_set_esp(void *p) { tss.esp0 = (uint32_t)p; }

extern void gdt_load(void);
extern void tss_load(void);

void gdt_init() {
  gdt_ptr.limit = sizeof(gdt) - 1;
  gdt_ptr.base = (uint32_t)&gdt;

  gdt_set_descr(0, 0, 0, 0, 0);
  gdt_set_descr(1, 0, 0xFFFFFFFF, 0x9A, 0x0C); // Kernel code segment
  gdt_set_descr(2, 0, 0xFFFFFFFF, 0x92, 0x0C); // Kernel data segment
  gdt_set_descr(3, 0, 0xFFFFFFFF, 0xFA, 0x0C); // User mode code segment
  gdt_set_descr(4, 0, 0xFFFFFFFF, 0xF2, 0x0C); // User mode data segment
  tss_set_descr(5, 0x10, 0x0);                 // Task state segment

  gdt_load();
  tss_load();
}
