#include <conf/config.h>
#include <i386/vmm.h>
#include <stdint.h>
#include <string.h>
#include <sys/param.h>
#include <sys/system.h>

#define PAGE_COUNT_MAX (MAX_MEM / PAGE_SIZE)
#define PAGE_BITMAP_SIZE ((MAX_MEM / PAGE_SIZE) / 8)

uint8_t vmm_bitmap[PAGE_BITMAP_SIZE];
unsigned int vmm_bitmap_index = 0;

void *vmm_alloc_page() {
  // Find first byte in bitmap that is not all allocated
  int count = 0;
  while (count < PAGE_BITMAP_SIZE && vmm_bitmap[vmm_bitmap_index] == 0xFF) {
    ++count;
    if (++vmm_bitmap_index >= PAGE_BITMAP_SIZE)
      vmm_bitmap_index = 0;
  }

  if (vmm_bitmap[vmm_bitmap_index] == 0xFF) {
    panic("Out of memory!");
  }

  // Find first bit in byte that is free
  uint8_t page_bit = 1;
  void *page_addr = (void *)(vmm_bitmap_index * PAGE_SIZE * 8);
  while (page_bit && (vmm_bitmap[vmm_bitmap_index] & page_bit)) {
    page_bit <<= 1;
    page_addr += PAGE_SIZE;
  }

  // Mark page as allocated
  vmm_bitmap[vmm_bitmap_index] |= page_bit;
  return page_addr;
}

int vmm_free_page(void *page_addr) {
  unsigned int page_no = (uint32_t)page_addr / PAGE_SIZE;
  unsigned int page_index = page_no >> 3;
  if (page_index >= PAGE_BITMAP_SIZE)
    panic("free page out of bounds");
  unsigned int bit_no = page_no % 8;
  uint8_t page_bit = 1 << bit_no;

  // Not allocated?
  if ((vmm_bitmap[page_index] & page_bit) == 0) {
    panic("page already free!");
  }

  // Mark page as free by masking out bit
  vmm_bitmap[page_index] &= ~page_bit;
  return 0;
}

int vmm_is_free(void *page_addr) {
  unsigned int page_no = (uint32_t)page_addr / PAGE_SIZE;
  unsigned int page_index = page_no >> 3;
  if (page_index >= PAGE_BITMAP_SIZE)
    return 0;
  uint8_t page_bit = 1 << (page_no & 3);

  // Allocated?
  return ((vmm_bitmap[page_index] & page_bit) == 0);
}

void *vmm_alloc_addr(void *page_addr) {
  unsigned int page_no = (uint32_t)page_addr / PAGE_SIZE;
  unsigned int page_index = page_no >> 3;
  if (page_index >= PAGE_BITMAP_SIZE)
    return NULL;
  uint8_t page_bit = 1 << (page_no & 3);

  // Not free?
  if ((vmm_bitmap[page_index] & page_bit) != 0)
    return NULL;

  // Mark page as allocated
  vmm_bitmap[page_index] |= page_bit;
  return page_addr;
}

void vmm_bitmap_init() {
  // Mark all pages unavailable, mem_detect() will mark available memory as free
  memset(vmm_bitmap, 0xFF, PAGE_BITMAP_SIZE);

  vmm_bitmap_index = 0;
}

void vmm_init() { vmm_bitmap_init(); }
