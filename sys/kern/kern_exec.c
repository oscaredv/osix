#include <i386/idt.h>
#include <i386/vmm.h>
#include <stdio.h>
#include <string.h>
#include <sys/inode.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/syslimits.h>
#include <sys/system.h>
#include <vm/pmap.h>

int sys_exec(const char *pathname, const char *argv[], const char *envp[]) {
  struct inode *inode = namei(pathname);
  if (inode == NULL) {
    return -1;
  }

  // Allocate and initialize new user stack, and copy data to it before we replace the old user pages
  uintptr_t ustack = (uintptr_t)vmm_alloc_page();
  uintptr_t user_esp = proc_init_ustack(ustack, argv, envp);

  // Free existing user pages
  proc_break(cur_proc, USER_TEXT);

  // Allocate pages for process text and data segments
  ilock(inode);
  proc_break(cur_proc, USER_TEXT + inode->i_size);

  // Clear signal mask and signal handlers, and signal dispatcher
  cur_proc->sigact.dispatcher = NULL;
  for (int s = 0; s < NSIG; s++) {
    cur_proc->sigact.handlers[s] = SIG_DFL;
  }

  // Read program file
  if (readi(inode, (void *)USER_TEXT, 0, inode->i_size) != inode->i_size)
    panic("exec read error!");

  iunlockput(inode);

  proc_init_kstack(cur_proc);

  // Destroy old ustack
  uintptr_t vaddr = USER_USTACK;
  uintptr_t paddr = pmap_extract(&cur_proc->pmap, vaddr);
  while (paddr != 0) {
    pmap_remove(&cur_proc->pmap, vaddr, vaddr + PAGE_SIZE);
    vaddr -= PAGE_SIZE;
    paddr = pmap_extract(&cur_proc->pmap, vaddr);
  }

  // Activate new ustack
  pmap_enter(&cur_proc->pmap, USER_USTACK, ustack, VM_PROT_ALL);

  proc_init_user_context(cur_proc, user_esp);

  pmap_flush();

  yield(); // Will not return
  return 0;
}
