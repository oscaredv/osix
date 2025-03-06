#ifndef __I386_MEM_H__
#define __I386_MEM_H__

#include <sys/multiboot.h>

void mem_detect(unsigned long magic, struct multiboot_info *mb);

#endif
