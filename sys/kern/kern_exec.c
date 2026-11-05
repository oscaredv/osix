#include <errno.h>
#include <i386/idt.h>
#include <i386/vmm.h>
#include <string.h>
#include <sys/inode.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/procinfo.h>
#include <sys/stat.h>
#include <sys/syslimits.h>
#include <sys/system.h>
#include <vm/pmap.h>

#define EXEC_MAGIC 0x0BB8
#define EXEC_SHEBANG 0x2123

struct exec_magic {
  uint16_t magic;
  char shebang[PATH_MAX];
};

int sys_exec(const char *pathname, const char *argv[], const char *envp[]) {
  struct inode *inode = NULL;
  char command[PATH_MAX];
  int error = nameiname(pathname, command, &inode);
  if (error != 0) {
    return error;
  }

  // Check if file is executable for user
  struct stat st;
  istat(inode, &st);

  // Check if the user has execute permission for the file
  // TODO: Support supplementary groups executable permission
  if (!(cur_proc->uid == 0 && (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) &&
      !(cur_proc->uid == st.st_uid && (st.st_mode & S_IXUSR)) &&
      !(cur_proc->gid == st.st_gid && (st.st_mode & S_IXGRP)) && !(st.st_mode & S_IXOTH)) {
    iput(inode);
    return -EACCES;
  }

  struct exec_magic exec = {};
  ilock(inode);
  // Read enough bytes to determine the type of executable
  int nread = readi(inode, &exec, 0, sizeof(exec) - 1);
  iunlock(inode);

  if (nread <= (int)sizeof(exec.magic)) {
    iput(inode);
    return -ENOEXEC;
  }

  const char *arg1 = NULL;
  if (exec.magic == EXEC_SHEBANG) {
    // Add argument for the script file to the interpreter
    arg1 = pathname;

    // Terminate the shebang line at newline
    for (int i = 0; i < PATH_MAX; i++) {
      if (exec.shebang[i] == '\n' || exec.shebang[i] == '\r') {
        exec.shebang[i] = 0;
      }
      if (exec.shebang[i] == 0)
        break;
    }

    // Re-resolve the inode now looking for the interpreter specified in the shebang line
    iput(inode);
    error = nameiname(exec.shebang, command, &inode);
    if (error != 0) {
      return error;
    }

    // Re-read the magic number, now from the interpreter file
    ilock(inode);
    nread = readi(inode, &exec, 0, sizeof(exec.magic));
    iunlock(inode);

    if (nread < (int)sizeof(exec.magic)) {
      iput(inode);
      return -ENOEXEC;
    }
  }

  // Verify that the executable has the correct magic number
  if (exec.magic != EXEC_MAGIC) {
    iput(inode);
    return -ENOEXEC;
  }

  // Set process command name
  strncpy(cur_proc->command, command, PROC_NAME_LEN - 1);
  cur_proc->command[PROC_NAME_LEN - 1] = 0;

  // Allocate and initialize new user stack, and copy data to it before we replace the old user pages
  uintptr_t ustack = (uintptr_t)vmm_alloc_page();
  uintptr_t user_esp = proc_init_ustack(ustack, argv, arg1, envp);

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
