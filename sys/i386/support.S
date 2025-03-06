/* Various functions implemented in x86 assembler code */

.extern gdt_ptr
.global gdt_load
gdt_load:
	lgdt (gdt_ptr)
	movl  $0x10, %eax
	movl  %eax, %ds
	movl  %eax, %es
	movl  %eax, %fs
	movl  %eax, %gs
	movl  %eax, %ss
	ljmp  $0x08, $.flush
.flush:
	ret

.global tss_load
tss_load:
	mov $0x28 | 3, %ax
	ltr %ax
	ret
