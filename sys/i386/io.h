#ifndef __I386_IO_H__
#define __I386_IO_H__

#include <stddef.h>
#include <stdint.h>

static inline void outb(uint8_t value, unsigned short port) { asm volatile("outb %0, %1" : : "a"(value), "Nd"(port)); }

static inline void outw(uint16_t value, unsigned short port) { asm volatile("outw %0, %1" : : "a"(value), "Nd"(port)); }

static inline void outl(uint32_t value, unsigned short port) { asm volatile("outl %0, %1" : : "a"(value), "Nd"(port)); }

static inline uint8_t inb(unsigned short port) {
  uint8_t value;
  asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

static inline uint16_t inw(unsigned short port) {
  uint16_t value;
  asm volatile("inw %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

static inline uint32_t inl(unsigned short port) {
  uint32_t value;
  asm volatile("inl %1, %0" : "=a"(value) : "Nd"(port));
  return value;
}

static inline void io_wait(void) { outb(0, 0x80); }

#endif
