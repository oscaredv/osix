#include <sys/syscall.h>

.section .text

.extern exit
.extern _end
.extern stdio_init

.global start
start:
    /* Set program break */
    movl $SYS_brk, %eax
    movl $_end, %ebx
    int $0x80

    /* Set environ variable */
    pop %ebx
    movl %ebx, environ

    call stdio_init

    call main
    /* return code from main in eax, pass it as argument to exit */
    push %eax
    call exit
