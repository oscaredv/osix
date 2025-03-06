#ifndef __VM_PMAP_H__
#define __VM_PMAP_H__

#include <i386/pmap.h>

#define VM_PROT_READ 1
#define VM_PROT_WRITE 2
#define VM_PROT_EXECUTE 4
#define VM_PROT_ALL (VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE)

struct proc;

void pmap_create(struct pmap *pmap);
void pmap_destroy(struct pmap *pmap);

void pmap_enter(struct pmap *pmap, unsigned long vaddr, unsigned long paddr, unsigned int prot);
void pmap_kenter(unsigned long vaddr, unsigned long paddr, unsigned int prot);
void pmap_remove(struct pmap *pmap, unsigned long vaddr_start, unsigned long vaddr_end);
void pmap_kremove(unsigned long vaddr_start, unsigned long vaddr_end);

unsigned long pmap_extract(struct pmap *pmap, unsigned long vaddr);

void pmap_zero_page(unsigned long paddr);
void pmap_copy_page(unsigned long src_paddr, unsigned long dst_paddr);

void pmap_activate(const struct proc *p);

void pmap_init(void);

#endif
