ENTRY(_start)

SYS_VADDR = 0x80000000;

SECTIONS {
	. = 0x100000;

	.boot BLOCK(4K) : ALIGN(4K) {
		*(.setup)
	}

	. += SYS_VADDR;

	_kernel_start = .;

	.text ALIGN(4K) : AT(ADDR(.text) - SYS_VADDR) {
		*(.multiboot) *(.text)
	}
	.rodata ALIGN(4K) : AT(ADDR(.rodata) - SYS_VADDR) {
		*(.rodata)
	}
	.data ALIGN(4K) : AT(ADDR(.data) - SYS_VADDR) {
		*(.data)
	}
	.bss ALIGN(4K) : AT(ADDR(.bss) - SYS_VADDR) {
		*(COMMON) *(.bss)
	}
	_kernel_end = .;

	/DISCARD/ : {
		*(.eh_frame .note.GNU-stack)
	}
}
