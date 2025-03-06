#ifndef __I386_GDT_H__
#define __I386_GDT_H__

void gdt_init();

void tss_set_esp(void *p);

#endif
