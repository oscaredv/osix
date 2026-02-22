#include <i386/param.h>
#include <i386/vmm.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/system.h>
#include <vm/pmap.h>

extern void *_kernel_end;
extern uint32_t sys_pg_dir[PAGE_DIR_LEN];
extern uint32_t sys_pg_table[PAGE_TABLE_LEN];

struct pmap kernel_pmap;

struct pmap *cur_pmap;

// Pointer to recursive page directory map that gives access to page directory and tables through virtual memory
uint32_t (*pte_map)[1024];
uint32_t *pg_dir;

void pmap_activate_pmap(struct pmap *pmap) {
  cur_pmap = pmap;
  asm volatile("movl %0,%%cr3" : : "r"(pmap->pg_dir_paddr));
}

void pmap_activate(struct proc *p) { pmap_activate_pmap(&p->pmap); }

void pmap_create(struct pmap *pmap) {
  // Alloc page directory
  pmap->pg_dir_paddr = vmm_alloc_page();

  // Temporarily map to kernel memory and clear it
  pmap_kenter(SYS_VADDR_TMP, (uintptr_t)pmap->pg_dir_paddr, VM_PROT_ALL);
  pmap_flush();
  memset((void *)SYS_VADDR_TMP, 0, PAGE_SIZE);

  // Map kernel page table to vaddr SYS_VADDR
  uint32_t *pg_dir = (uint32_t *)SYS_VADDR_TMP;
  pg_dir[SYS_VADDR_INDEX] = V2P(sys_pg_table) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

  // Recursive mapping of page directory to itself, allows access to page tables through virtual memory
  pg_dir[PTE_MAP_INDEX] = (uint32_t)pmap->pg_dir_paddr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;

  // Unmap from kernel memory
  pmap_kremove(SYS_VADDR_TMP, SYS_VADDR_TMP + PAGE_SIZE);
}

void pmap_destroy(struct pmap *pmap) {
  struct pmap *old_pmap = cur_pmap;
  if (cur_pmap != pmap)
    pmap_activate_pmap(pmap);

  // Free all user mode pages
  for (unsigned int pd_index = 0; pd_index < SYS_VADDR_INDEX; pd_index++) {
    if (pg_dir[pd_index] & PAGE_PRESENT) {
      // Unmap and free page table
      // unsigned long pg_table = pmap->pg_dir[pd_index] & PAGE_MASK;
      unsigned long pg_table = pg_dir[pd_index] & PAGE_MASK;
      vmm_free_page((void *)pg_table);
    }
  }

  // Get out of the pmap before we destroy it
  if (cur_pmap != old_pmap)
    pmap_activate_pmap(old_pmap);

  // Unmap and free page directory
  vmm_free_page(pmap->pg_dir_paddr);
  pmap->pg_dir_paddr = NULL;
}

unsigned long pmap_extract(struct pmap *pmap, unsigned long vaddr) {
  struct pmap *old_pmap = cur_pmap;
  if (cur_pmap != pmap)
    pmap_activate_pmap(pmap);

  unsigned long pd_index = vaddr >> PAGE_DIR_SHIFT;

  // Out of page directory range, or page table not mapped
  if (pd_index >= PAGE_DIR_LEN || !(pg_dir[pd_index] & PAGE_PRESENT)) {
    if (cur_pmap != old_pmap)
      pmap_activate_pmap(old_pmap);
    return 0;
  }

  // Get the page table
  unsigned long pt_index = (vaddr & PAGE_TABLE_MASK) >> PAGE_SHIFT;

  // Get page table entry
  unsigned long entry = pte_map[pd_index][pt_index];

  if (cur_pmap != old_pmap)
    pmap_activate_pmap(old_pmap);

  if ((entry & PAGE_PRESENT) == 0)
    return 0; // not mapped

  return entry & PAGE_MASK;
}

void pmap_kenter(unsigned long vaddr, unsigned long paddr, unsigned int prot) {
  pmap_enter(&kernel_pmap, vaddr, paddr, prot);
}

void pmap_enter_raw(struct pmap *pmap, unsigned long vaddr, unsigned long paddr, unsigned int prot) {
  unsigned long pd_index = vaddr >> PAGE_DIR_SHIFT;
  unsigned long pt_index = (vaddr & PAGE_TABLE_MASK) >> PAGE_SHIFT;

  pte_map[pd_index][pt_index] = paddr | PAGE_PRESENT;

  if (prot & VM_PROT_WRITE)
    pte_map[pd_index][pt_index] |= PAGE_WRITABLE;

  if (pmap != &kernel_pmap)
    pte_map[pd_index][pt_index] |= PAGE_USER;
}

void pmap_enter(struct pmap *pmap, unsigned long vaddr, unsigned long paddr, unsigned int prot) {
  struct pmap *old_pmap = cur_pmap;
  if (cur_pmap != pmap)
    pmap_activate_pmap(pmap);

  unsigned long pd_index = vaddr >> PAGE_DIR_SHIFT;

  // Out of page directory range, or page table not mapped
  if (pd_index >= PAGE_DIR_LEN)
    panic("pmap_enter");

  if ((pg_dir[pd_index] & PAGE_PRESENT) == 0) {
    // Alloc page table
    unsigned long pg_table_paddr = (unsigned long)vmm_alloc_page();

    // Map page table to kernel memory and clear it
    pmap_zero_page(pg_table_paddr);

    // Map user page table to vaddr 0, and kernel page table to vaddr SYS_VADDR
    pg_dir[pd_index] = pg_table_paddr | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
  }

  pmap_enter_raw(pmap, vaddr, paddr, prot);

  if (cur_pmap != old_pmap)
    pmap_activate_pmap(old_pmap);
}

void pmap_kremove(unsigned long vaddr_start, unsigned long vaddr_end) {
  pmap_remove(&kernel_pmap, vaddr_start, vaddr_end);
}

void pmap_remove(struct pmap *pmap, unsigned long vaddr_start, unsigned long vaddr_end) {
  struct pmap *old_pmap = cur_pmap;
  if (cur_pmap != pmap)
    pmap_activate_pmap(pmap);

  for (unsigned long vaddr = vaddr_start; vaddr + PAGE_SIZE <= vaddr_end; vaddr += PAGE_SIZE) {
    unsigned long pd_index = vaddr >> PAGE_DIR_SHIFT;

    if (pd_index >= PAGE_DIR_LEN)
      break;

    unsigned long pt_index = (vaddr & PAGE_TABLE_MASK) >> PAGE_SHIFT;
    pte_map[pd_index][pt_index] = 0;
  }

  if (cur_pmap != old_pmap)
    pmap_activate_pmap(old_pmap);
}

void pmap_zero_page(unsigned long paddr) {
  // Temporarily map to kernel virtual memory
  pmap_kenter(SYS_VADDR_TMP, paddr, VM_PROT_WRITE);
  pmap_flush();

  // Zero page of memory
  memset((void *)SYS_VADDR_TMP, 0, PAGE_SIZE);

  // Unmap from kernel memory
  pmap_kremove(SYS_VADDR_TMP, SYS_VADDR_TMP + PAGE_SIZE);
}

void pmap_copy_page(unsigned long src_paddr, unsigned long dst_paddr) {
  // Temporarily map to kernel virtual memory
  pmap_kenter(SYS_VADDR_TMP, src_paddr, VM_PROT_READ);
  pmap_kenter(SYS_VADDR_TMP2, dst_paddr, VM_PROT_READ | VM_PROT_WRITE);
  pmap_flush();

  // Copy page
  memcpy((void *)SYS_VADDR_TMP2, (void *)SYS_VADDR_TMP, PAGE_SIZE);

  // Unmap from kernel memory
  pmap_kremove(SYS_VADDR_TMP, SYS_VADDR_TMP + PAGE_SIZE);
  pmap_kremove(SYS_VADDR_TMP2, SYS_VADDR_TMP2 + PAGE_SIZE);
}

void pmap_init() {
  kernel_pmap.pg_dir_paddr = (void *)V2P(sys_pg_dir);
  cur_pmap = &kernel_pmap;

  // Recursive mapping of page directory to itself, allows access to page tables through virtual memory
  sys_pg_dir[PTE_MAP_INDEX] = V2P(sys_pg_dir) | PAGE_PRESENT | PAGE_WRITABLE | PAGE_USER;
  pmap_flush();

  // Set global pointers to page tables and directory through recursive page directory mapping
  pte_map = (uint32_t (*)[1024])PTE_MAP;
  pg_dir = (uint32_t *)pte_map[PTE_MAP_INDEX];

  // Unmap virtual memory before kernel
  pmap_remove(&kernel_pmap, SYS_VADDR, SYS_VADDR + 0xB8000);

  // Unmap virtual memory after kernel
  uintptr_t kernel_end_page_addr = (uintptr_t)&_kernel_end;
  kernel_end_page_addr = (kernel_end_page_addr + PAGE_SIZE) & PAGE_MASK;
  pmap_remove(&kernel_pmap, kernel_end_page_addr, P2V(PAGE_SIZE * PAGE_TABLE_LEN));
}
