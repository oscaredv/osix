#include <i386/mem.h>
#include <i386/param.h>
#include <i386/vmm.h>
#include <stdint.h>
#include <stdio.h>

extern void *_kernel_end;

void mem_detect(unsigned long magic, struct multiboot_info *mb) {
  unsigned long real_mem = 0;
  unsigned long avail_mem = 0;

  uintptr_t kernel_end = V2P(&_kernel_end);

  if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
    printf("BAD MULTIBOOT MAGIC NUMBER 0x%x\n", magic);
  }

  if (mb->flags & MULTIBOOT_INFO_MEM_MAP) {
    unsigned long offset = 0;

    while (offset < mb->mmap_length) {
      multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)P2V(mb->mmap_addr + offset);

      if (mmap->type == MULTIBOOT_MEMORY_AVAILABLE) {
        unsigned long page_aligned_addr = (mmap->addr + PAGE_SIZE - 1) & PAGE_MASK;

        while ((page_aligned_addr + PAGE_SIZE) < (mmap->addr + mmap->len)) {
          if (page_aligned_addr > kernel_end) {
            // Mark page as available
            vmm_free_page((void *)page_aligned_addr);
            avail_mem += PAGE_SIZE;
          }
          page_aligned_addr += PAGE_SIZE;
        }
      }

      real_mem += mmap->len;
      offset += mmap->size + sizeof(mmap->size);
    }
  } else if (mb->flags & MULTIBOOT_INFO_MEMORY) {
    real_mem = (mb->mem_lower + mb->mem_upper) << 10;
    // TODO: mb->mem_upper contains available memory
  } else {
    // TODO: read CMOS or probe memory
  }

  printf("real mem = %d (%d MB)\n", real_mem, real_mem >> 20);
  printf("avail mem = %d (%d MB)\n", avail_mem, avail_mem >> 20);
}
