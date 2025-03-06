#ifndef __I386_PARAM_H__
#define __I386_PARAM_H__

#define PAGE_SIZE 4096
#define PAGE_MASK 0xFFFFF000
#define PAGE_SHIFT 12
#define PAGE_DIR_SHIFT 22

#define PAGE_DIR_LEN 1024
#define PAGE_TABLE_LEN 1024
#define PAGE_TABLE_MASK (PAGE_SIZE * PAGE_TABLE_LEN - 1)

#define PAGE_PRESENT 1
#define PAGE_WRITABLE 2
#define PAGE_USER 4
#define PAGE_ACCESSED 32
#define PAGE_DIRTY 64

#define SYS_VADDR 0x80000000
#define SYS_VADDR_INDEX (SYS_VADDR >> 22)

#define P2V(addr) ((unsigned long)(addr) + SYS_VADDR)
#define V2P(addr) ((unsigned long)(addr) - SYS_VADDR)

#define STACK_SIZE 4096

#define USER_TEXT 0x100000
#define USER_USTACK 0x1FF000

#endif
