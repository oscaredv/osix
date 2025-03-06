#include <i386/param.h>
#include <i386/vmm.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/inode.h>
#include <sys/system.h>
#include <vm/pmap.h>

void proc_exit(struct proc *p, int status) {
  // Orphaned children will be adopted by init
  for (int i = 0; i < NPROC; i++) {
    if (proc[i].parent == p) {
      proc[i].parent = &proc[0]; // Assume init is the first process in the process table
      if (proc[i].state == STATE_ZOMBIE) {
        // Wake up new parent so it can clean its newly adopted zombie
        wakeup(proc[i].parent);
      }
    }
  }

  // Free all user mode pages
  proc_break(p, USER_TEXT);

  // Free stack
  unsigned long vaddr = USER_USTACK;
  unsigned long paddr = pmap_extract(&p->pmap, vaddr);
  while (paddr != 0) {
    pmap_remove(&p->pmap, vaddr, vaddr + PAGE_SIZE);
    vaddr -= PAGE_SIZE;
    paddr = pmap_extract(&p->pmap, vaddr);
  }

  // Close all open files
  for (int f = 0; f < NFILE; f++) {
    if (p->ofile[f]) {
      file_close(p->ofile[f]);
    }
  }

  // Switch to parents page directory before deallocating ours
  pmap_activate(p->parent);

  // Free page table and directory
  pmap_destroy(&p->pmap);

  // wakeup parent if it's waiting for this process
  wakeup(p->parent);

  // Drop reference to working directory
  iput(p->cwd);
  p->cwd = NULL;

  p->exit_status = status;
  p->state = STATE_ZOMBIE;
  yield(); // Will not return
}

void exit(int status) { proc_exit(cur_proc, status); }
