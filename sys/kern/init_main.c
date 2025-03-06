// Could all dev_init functions be in common include so we dont have to include them all?
// Or put init functions as function pointers in devsw
#include <dev/if_re.h>
#include <dev/pci.h>
#include <i386/cons.h>
#include <i386/idt.h>
#include <i386/kb.h>
#include <i386/pic.h>
#include <i386/pit.h>
#include <i386/uart.h>
#include <i386/vmm.h>
#include <i386/wd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/conf.h>
#include <sys/fs.h>
#include <sys/multiboot.h>
#include <sys/proc.h>
#include <sys/system.h>

// Move to system.h ?
#include <i386/cpu.h>
#include <i386/gdt.h>
#include <i386/mem.h>
#include <i386/rtc.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/tty.h>
#include <vm/pmap.h>

void main(unsigned long magic, struct multiboot_info *mb) {
  cn_init();
  printf("%s\n", version);
  vmm_init();
  mem_detect(magic, mb);

  cpu_detect();

  pic_init();
  gdt_init();
  idt_init();
  pmap_init();

  pit_init();
  rtc_read();

  kb_init();
  tty_init();

  uart_init();
  pci_probe();
  buf_init();
  wd_init();
  re_probe();

  inode_init();
  file_init();

  // Setup process table
  proc_init();

  mount(ROOT_DEV);

  // Create first process
  proc_create("/etc/init");

  sched(); // Scheduler loop
}
