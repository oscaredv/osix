#include <i386/param.h>
#include <sys/multiboot.h>

.section .boot, "a"

/* Multiboot header. */
    .align 4
    /* Header magic number used for locating the multiboot header */
    .long MULTIBOOT_HEADER_MAGIC
    /* Multiboot flags - align to page boundaries and provide memory map info */
    .long MULTIBOOT_PAGE_ALIGN | MULTIBOOT_MEMORY_INFO
    /* Multiboot header checksum */
    .long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_PAGE_ALIGN + MULTIBOOT_MEMORY_INFO)

/* Initial kernel stack - used until first user mode process starts */
    .align 4
    .skip 4096
stack0:

/* Kernel entry point - sets up stack, pushes args for main(), enables paging and jumps to higher half */
.global _start
_start:
    cli
    /* Setup stack */
    mov $stack0, %esp

	/* push multiboot struct parameters */
    add $SYS_VADDR, %ebx
    push %ebx
    push %eax

    /* Enable paging and write protection */
    mov $sys_pg_dir, %eax
    sub $SYS_VADDR, %eax
    mov %eax, %cr3
    mov %cr0, %eax
    or $0x80010000, %eax
    mov %eax, %cr0

    lea higher_half, %ecx
    jmp *%ecx

.section .text

/* Kernel is now running on virtual address space at SYS_VADDR - adjust stack and page directory, call main() */
higher_half:

    /* Move stack pointer to higher half */
    add $SYS_VADDR, %esp

    /* Remove mapping of first 4 megabytes */
    movl $0, sys_pg_dir

    /* Jump to C */
    call main

    /* Kernel should never return, halt if it does */
    cli
    hlt

.section .data

/* Temporary page directory, first 4 megabytes are identity mapped both at offset 0 and offset SYS_VADDR. After jumping
   to higher half of memory, the mapping at offset 0 is removed. This page directory is only used until first user
   space process starts. After that the kernel doesn't have it's own page directory, but has a page table in each
   process's page directory.
*/
    .align 4096
.global sys_pg_dir
sys_pg_dir:
    .long sys_pg_table - SYS_VADDR + PAGE_PRESENT + PAGE_WRITABLE
    .fill (SYS_VADDR_INDEX -1), 4, 0
    .long sys_pg_table - SYS_VADDR + PAGE_PRESENT + PAGE_WRITABLE
    .fill PAGE_DIR_LEN - 1 - SYS_VADDR_INDEX, 4, 0

/* Kernels page table, this table will be add offset SYS_VADDR (index=SYS_VADDR_INDEX) of all page directories */
.global sys_pg_table
sys_pg_table:
    .rept PAGE_TABLE_LEN
        .long ((. - sys_pg_table) * PAGE_TABLE_LEN) + PAGE_PRESENT + PAGE_WRITABLE
    .endr
