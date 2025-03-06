#include <i386/idt.h>
#include <i386/vmm.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/inode.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/system.h>
#include <vm/pmap.h>

void copy_page(struct proc *src_proc, unsigned long src_vaddr, struct proc *dst_proc, unsigned long dst_vaddr) {
  if (src_vaddr >= SYS_VADDR || dst_vaddr >= SYS_VADDR)
    panic("copy_page");

  // Find physical page addresses
  unsigned long src = pmap_extract(&src_proc->pmap, (unsigned long)src_vaddr);
  unsigned long dst = pmap_extract(&dst_proc->pmap, (unsigned long)dst_vaddr);

  pmap_copy_page(src, dst);
}

int fork() {
  struct proc *child = proc_alloc();
  if (child == NULL) {
    return -1;
  }

  child->parent = cur_proc;

  // Inherit process group from parent
  child->pgrp = cur_proc->pgrp;

  // If process group is 0, start a new group / session
  if (child->pgrp == 0)
    child->pgrp = child->pid;

  // Copy parents interrupt frame to kstack for popping into registers
  *child->context = *cur_proc->context;
  child->context->eax = 0; // fork() will return 0 in the child process

  // Inherit signal mask and signal handlers, and signal dispatcher
  child->sig_mask = cur_proc->sig_mask;
  child->sigact.dispatcher = cur_proc->sigact.dispatcher;
  for (int s = 0; s < NSIG; s++) {
    child->sigact.handlers[s] = cur_proc->sigact.handlers[s];
  }

  // Alloc page directory & table
  pmap_create(&child->pmap);

  // Alloc, map and copy user mode stack
  uintptr_t ustack = (uintptr_t)vmm_alloc_page();
  pmap_enter(&child->pmap, USER_USTACK, ustack, VM_PROT_ALL);
  copy_page(cur_proc, USER_USTACK, child, USER_USTACK);

  // Alloc user mode pages
  proc_break(child, cur_proc->brk);

  // TODO: Should use light weight copy
  // Copy user mode pages
  for (unsigned long page_addr = USER_TEXT; page_addr < (unsigned long)child->brk; page_addr += PAGE_SIZE) {
    copy_page(cur_proc, page_addr, child, page_addr);
  }

  child->cwd = idup(cur_proc->cwd);

  // Inherit all open files
  for (int f = 0; f < NFILE; f++) {
    if (cur_proc->ofile[f]) {
      child->ofile[f] = file_dup(cur_proc->ofile[f]);
    }
  }

  // Child process is now ready to run
  child->state = STATE_RUNNING;

  pmap_activate(cur_proc);

  return child->pid;
}
