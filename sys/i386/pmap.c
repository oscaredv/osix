#include <i386/param.h>
#include <i386/vmm.h>
#include <string.h>
#include <sys/proc.h>
#include <vm/pmap.h>

#include <stdio.h>
#include <sys/system.h>

extern void *_kernel_end;
extern uint32_t sys_pg_dir[PAGE_DIR_LEN];
extern uint32_t sys_pg_table[PAGE_TABLE_LEN];

struct pmap kernel_pmap;

void pmap_activate(const struct proc *p) { asm volatile("movl %0,%%cr3" : : "r"(p->pmap.pg_dir_paddr)); }

void pmap_create(struct pmap *pmap) {
  // Alloc page directory
  pmap->pg_dir_paddr = vmm_alloc_page();
  pmap->pg_dir = (uint32_t *)P2V(pmap->pg_dir_paddr);

  // Map page directory to kernel memory and clear it
  pmap_kenter(P2V(pmap->pg_dir_paddr), (uintptr_t)pmap->pg_dir_paddr, VM_PROT_ALL);
  memset(pmap->pg_dir, 0, PAGE_SIZE);

  // Map kernel page table to vaddr SYS_VADDR
  pmap->pg_dir[SYS_VADDR_INDEX] = V2P(sys_pg_table) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
}

void pmap_destroy(struct pmap *pmap) {
  // Unmap from kernel memory
  for (unsigned int pd_index = 0; pd_index < SYS_VADDR_INDEX; pd_index++) {
    if (pmap->pg_dir[pd_index] & PAGE_PRESENT) {
      // Unmap and free page table
      unsigned long pg_table = pmap->pg_dir[pd_index] & PAGE_MASK;
      pmap_remove(pmap, P2V(pg_table), P2V(pg_table + PAGE_SIZE));
      vmm_free_page((void *)pg_table);
    }
  }

  // Unmap and free page directory
  pmap_remove(pmap, P2V(pmap->pg_dir_paddr), P2V(pmap->pg_dir_paddr) + PAGE_SIZE);
  vmm_free_page(pmap->pg_dir_paddr);
  pmap->pg_dir_paddr = NULL;
}

unsigned long pmap_extract(struct pmap *pmap, unsigned long vaddr) {
  unsigned long pd_index = vaddr >> PAGE_DIR_SHIFT;

  // Out of page directory range, or page table not mapped
  if (pd_index >= PAGE_DIR_LEN || !pmap->pg_dir[pd_index] & PAGE_PRESENT)
    return 0;

  // Get the page table
  uint32_t *pg_table = (uint32_t *)V2P(pmap->pg_dir[pd_index] & PAGE_MASK);

  unsigned long pt_index = (vaddr & PAGE_TABLE_MASK) >> PAGE_SHIFT;

  if ((pg_table[pt_index] & PAGE_PRESENT) == 0)
    return 0; // not mapped

  return pg_table[pt_index] & PAGE_MASK;
}

void pmap_kenter(unsigned long vaddr, unsigned long paddr, unsigned int prot) {
  pmap_enter(&kernel_pmap, vaddr, paddr, prot);
}

void pmap_enter(struct pmap *pmap, unsigned long vaddr, unsigned long paddr, unsigned int prot) {
  unsigned long pd_index = vaddr >> PAGE_DIR_SHIFT;

  // Out of page directory range, or page table not mapped
  if (pd_index >= PAGE_DIR_LEN)
    panic("pmap_enter");

  if ((pmap->pg_dir[pd_index] & PAGE_PRESENT) == 0) {
    // Alloc page table
    unsigned long pg_table_paddr = (unsigned long)vmm_alloc_page();

    // Map page table to kernel memory and clear it
    pmap_kenter(P2V(pg_table_paddr), pg_table_paddr, VM_PROT_ALL);
    memset((void *)P2V(pg_table_paddr), 0, PAGE_SIZE);

    // Map user page table to vaddr 0, and kernel page table to vaddr SYS_VADDR
    pmap->pg_dir[pd_index] = pg_table_paddr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
  }

  // Get the page table
  uint32_t *pg_table = (uint32_t *)V2P(pmap->pg_dir[pd_index] & PAGE_MASK);
  unsigned long pt_index = (vaddr & PAGE_TABLE_MASK) >> PAGE_SHIFT;

  pg_table[pt_index] = paddr | PAGE_PRESENT;
  if (prot & VM_PROT_WRITE)
    pg_table[pt_index] |= PAGE_WRITABLE;

  if (pmap != &kernel_pmap)
    pg_table[pt_index] |= PAGE_USER;
}

void pmap_kremove(unsigned long vaddr_start, unsigned long vaddr_end) {
  pmap_remove(&kernel_pmap, vaddr_start, vaddr_end);
}

void pmap_remove(struct pmap *pmap, unsigned long vaddr_start, unsigned long vaddr_end) {
  for (unsigned long vaddr = vaddr_start; vaddr + PAGE_SIZE <= vaddr_end; vaddr += PAGE_SIZE) {
    unsigned long pd_index = vaddr >> PAGE_DIR_SHIFT;

    if (pd_index >= PAGE_DIR_LEN)
      break;

    // Get the page table
    uint32_t *pg_table = (uint32_t *)V2P(pmap->pg_dir[pd_index] & PAGE_MASK);
    unsigned long pt_index = (vaddr & PAGE_TABLE_MASK) >> PAGE_SHIFT;
    pg_table[pt_index] = 0;
  }
}

void pmap_zero_page(unsigned long paddr) {
  // Map to kernel virtual memory
  pmap_kenter(P2V(paddr), paddr, VM_PROT_WRITE);

  // Zero page of memory
  memset((void *)P2V(paddr), 0, PAGE_SIZE);

  // Unmap from kernel memory
  pmap_kremove(P2V(paddr), P2V(paddr + PAGE_SIZE));
}

void pmap_copy_page(unsigned long src_paddr, unsigned long dst_paddr) {
  // Map to kernel virtual memory
  pmap_kenter(P2V(src_paddr), src_paddr, VM_PROT_READ);
  pmap_kenter(P2V(dst_paddr), dst_paddr, VM_PROT_READ | VM_PROT_WRITE);

  // Copy page
  memcpy((void *)P2V(dst_paddr), (void *)P2V(src_paddr), PAGE_SIZE);

  // Unmap from kernel memory
  pmap_kremove(P2V(src_paddr), P2V(src_paddr + PAGE_SIZE));
  pmap_kremove(P2V(dst_paddr), P2V(dst_paddr + PAGE_SIZE));
}

void pmap_init() {
  kernel_pmap.pg_dir = sys_pg_dir;
  kernel_pmap.pg_dir_paddr = (void *)V2P(sys_pg_dir);

  // Unmap virtual memory before kernel
  pmap_remove(&kernel_pmap, SYS_VADDR, SYS_VADDR + 0xB8000);

  // Unmap virtual memory after kernel
  uintptr_t kernel_end_page_addr = (uintptr_t)&_kernel_end;
  kernel_end_page_addr = (kernel_end_page_addr + PAGE_SIZE) & PAGE_MASK;
  pmap_remove(&kernel_pmap, kernel_end_page_addr, P2V(PAGE_SIZE * PAGE_TABLE_LEN));
}
