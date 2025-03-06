OUTPUT_FORMAT("binary")
ENTRY(start)
phys = 0x100000;

SEARCH_DIR(../obj/usr/lib)

STARTUP(crt0.o)

SECTIONS {
  .text phys : AT(phys) {
    _text = .;
    *(.text)
    *(.rodata)
    . = ALIGN(4096);
  }
  .data : AT(phys + (_data - _text)) {
    _data = .;
    *(.data)
    . = ALIGN(4096);
  }

  .bss : AT(phys + (_bss - _text)) {
    _bss = .;
    *(.bss)
    . = ALIGN(4096);
  }
  _end = .;
}
