#ifndef __I386_VM_H__
#define __I386_VM_H__

void vmm_init();

void *vmm_alloc_page();
int vmm_free_page(void *page_addr);
int vmm_is_free(void *page_addr);
void *vmm_alloc_addr(void *page_addr);

#endif
