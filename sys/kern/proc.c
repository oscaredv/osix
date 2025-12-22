#include <errno.h>
#include <i386/idt.h>
#include <i386/pit.h>
#include <i386/vmm.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/inode.h>
#include <sys/param.h>
#include <sys/proc.h>
#include <sys/system.h>
#include <vm/pmap.h>

struct proc proc[NPROC];
struct proc *cur_proc;

pid_t nextpid = 1;

uint8_t kstack[NPROC][STACK_SIZE] __attribute__((aligned(PAGE_SIZE)));

extern void sysleave(void);

void proc_init_kstack(struct proc *p) {
  // Setup kernel stack
  p->esp0 = p->kstack + PAGE_SIZE;

  // Allocate interrupt frame on kstack
  p->esp0 -= sizeof(struct interrupt_frame);
  p->context = p->esp0;

  p->esp0 -= sizeof(uintptr_t);
  void *sysleave_function = sysleave;
  memcpy(p->esp0, &sysleave_function, sizeof(sysleave_function));

  // swtch() pops edi, esi, ebx and ebp
  p->esp0 -= 4 * sizeof(uintptr_t);
}

uintptr_t proc_init_ustack(uintptr_t ustack, const char *argv[], const char *parent_envp[]) {
  // Temporary map stack to kernel memory
  pmap_kenter(P2V(ustack), ustack, VM_PROT_ALL);

  // Clear stack
  uintptr_t esp = P2V(ustack);
  memset((void *)esp, 0, PAGE_SIZE);

  // The new stack is mapped to a temporary address, this offset is used to calculate actual addresses
  uintptr_t esp_offset = esp - USER_USTACK;

  // Move esp to stack top
  esp += PAGE_SIZE;

  // Count environment variables
  int envc = 0;
  while (parent_envp[envc] != NULL)
    ++envc;

  // Copied environmens vector
  const char *envp[envc + 1]; // Make room for terminating NULL pointer

  for (int e = 0; e < envc; e++) {
    int env_len = strlen(parent_envp[e]);
    // Align string length to four bytes boundary
    int env_stack_len = (env_len + sizeof(uintptr_t)) & ~(sizeof(uintptr_t) - 1);

    // Copy environment variable to stack
    esp -= env_stack_len;
    strncpy((char *)esp, parent_envp[e], env_stack_len);

    // Store address to argument in new arguments vector
    envp[e] = (const char *)(esp - esp_offset);
  }
  envp[envc] = NULL;

  // Copy environment vector to stack
  esp -= sizeof(envp);
  memcpy((void *)esp, envp, sizeof(envp));

  // Remember address of environment vector
  uintptr_t envp_addr = esp - esp_offset;

  // Count arguments
  int argc = 0;
  while (argv[argc] != NULL)
    ++argc;

  // Copied arguments vector
  const char *argp[argc];

  for (int a = 0; a < argc; a++) {
    int arg_len = strlen(argv[a]);
    // Align string length to four bytes boundary
    int arg_stack_len = (arg_len + sizeof(uintptr_t)) & ~(sizeof(uintptr_t) - 1);

    // Copy argument to stack
    esp -= arg_stack_len;
    strncpy((char *)esp, argv[a], arg_stack_len);

    // Store address to argument in new arguments vector
    argp[a] = (const char *)(esp - esp_offset);
  }

  // Push arguments vector to stack
  esp -= sizeof(argp);
  memcpy((void *)esp, argp, sizeof(argp));
  uintptr_t argp_addr = esp - esp_offset;

  // Push environ vector to stack as possible third argument for main()
  esp -= sizeof(envp_addr);
  memcpy((void *)esp, &envp_addr, sizeof(envp_addr));

  // Push argv parameter to stack as argument for main()
  esp -= sizeof(argp_addr);
  memcpy((void *)esp, &argp_addr, sizeof(argp_addr));

  // Push argc parameter to stack as argument for main()
  esp -= sizeof(argc);
  memcpy((void *)esp, &argc, sizeof(argc));

  // Push environ vector to stack, again, this time for crt0 to pop and set environ
  esp -= sizeof(envp_addr);
  memcpy((void *)esp, &envp_addr, sizeof(envp_addr));

  // Unmap from temporary address
  pmap_kremove(P2V(ustack), P2V(ustack + PAGE_SIZE));
  return esp - esp_offset;
}

struct proc *proc_alloc() {
  struct proc *p = NULL;
  for (int i = 0; i < NPROC; i++) {
    if (proc[i].state == STATE_NULL) {
      p = &proc[i];

      // Make sure process table entry is cleared incase it's re-used
      memset(p, 0, sizeof(struct proc));
      p->kstack = &kstack[i];
      break;
    }
  }

  // Process table full?
  if (p == NULL)
    return NULL;

  // Are we out of PIDs
  if (nextpid == 0x7FFF)
    return NULL;

  p->cwd = NULL;
  p->wchan = NULL;
  p->state = STATE_IDLE;
  p->pid = nextpid++;
  p->brk = USER_TEXT;
  for (int s = 0; s < NSIG; s++) {
    p->sigact.handlers[s] = SIG_DFL;
  }
  proc_init_kstack(p);

  return p;
}

void proc_init_user_context(struct proc *p, uintptr_t user_esp) {
  // Initialize registers for user mode
  p->context->cs = 0x18 | 3;       // User mode code segment, ring 3
  p->context->ds = 0x20 | 3;       // User mode data segment, ring 3
  p->context->ss = p->context->ds; // Stack segment same as data segment
  p->context->esp = 0;             // USER_USTACK + STACK_SIZE;
  p->context->eflags = 0x200;      // Enable interrupts in user mode
  p->context->eip = 0x100000;      // User mode text starts at 1M
  p->context->user_esp = user_esp; // Top of user mode stack
}

void proc_create(const char *path) {
  struct proc *p = proc_alloc();

  if (p == NULL)
    panic("Process table full!");

  pmap_create(&p->pmap);
  pmap_activate(p);

  // Alloc user mode stack
  uintptr_t ustack = (uintptr_t)vmm_alloc_page();
  pmap_zero_page(ustack);

  const char *argv[] = {path, NULL};
  const char *envp[] = {NULL};
  uintptr_t user_esp = proc_init_ustack(ustack, argv, envp);

  pmap_enter(&p->pmap, USER_USTACK, ustack, VM_PROT_ALL);

  // Find program
  struct inode *inode = namei(path);
  if (inode == NULL)
    panic("init not found!");

  ilock(inode);

  // Allocate text segment and load program
  proc_break(p, USER_TEXT + inode->i_size);
  if (readi(inode, (void *)USER_TEXT, 0, inode->i_size) != inode->i_size)
    panic("init load error");

  iunlockput(inode);

  proc_init_user_context(p, user_esp);

  // Set current working directory
  p->cwd = namei("/");

  p->state = STATE_RUNNING;
}

void wakeup(void *wchan) {
  for (int p = 0; p < NPROC; p++) {
    if (proc[p].state == STATE_SLEEP && proc[p].wchan == wchan) {
      proc[p].state = STATE_RUNNING;
      proc[p].wchan = NULL;
    }
  }
}

void sleep(void *wchan) {
  if (cur_proc == NULL)
    panic("sleep: no current process");

  cur_proc->wchan = wchan;
  cur_proc->state = STATE_SLEEP;
  yield();
}

pid_t wait(int *status) {
  int child_count = 0;
  do {
    child_count = 0;
    for (int p = 0; p < NPROC; p++) {
      if (proc[p].state != STATE_NULL && proc[p].parent == cur_proc) {
        child_count++;
        if (proc[p].state == STATE_ZOMBIE) {
          // Collect exit status, free zombie, and return pid
          if (status != NULL) {
            *status = proc[p].exit_status;
          }
          proc[p].state = STATE_NULL;
          return proc[p].pid;
        }
      }
    }
    if (child_count > 0) {
      if (signal_pending(cur_proc))
        return -EINTR;

      // Wait for any child to exit
      sleep(cur_proc);
    }
  } while (child_count > 0);

  // Process has no child processes
  return -ECHILD;
}

// sleep() system call
unsigned int sys_sleep(unsigned int seconds) {
  unsigned int wakeup_time = pit_ticks + (PIT_FREQ * seconds);

  // Sleep until wakeup time
  sleep((void *)wakeup_time);

  // Check if we woke up early
  unsigned int rem_ticks = 0;
  if (pit_ticks < wakeup_time) {
    rem_ticks = wakeup_time - pit_ticks;
  }

  return rem_ticks / PIT_FREQ;
}

void proc_init() {
  for (int p = 0; p < NPROC; p++) {
    proc[p].state = STATE_NULL;
  }
}

unsigned long proc_break(struct proc *p, unsigned long brk) {
  if (brk == 0)
    return p->brk;

  if (brk < USER_TEXT)
    brk = USER_TEXT;

  // Round size up to next page boundry
  brk = (brk + PAGE_SIZE - 1) & PAGE_MASK;

  if (brk > p->brk) {
    // Increase program size
    for (unsigned long vaddr = p->brk; vaddr < brk; vaddr += PAGE_SIZE) {
      unsigned long paddr = (unsigned long)vmm_alloc_page();
      pmap_zero_page(paddr);
      pmap_enter(&p->pmap, vaddr, paddr, VM_PROT_ALL);
    }
    pmap_flush();
  } else if (brk < p->brk) {
    // Decrease program size
    for (unsigned long vaddr = brk; vaddr < p->brk; vaddr += PAGE_SIZE) {
      unsigned long paddr = pmap_extract(&p->pmap, vaddr);
      pmap_remove(&p->pmap, vaddr, vaddr + PAGE_SIZE);
      vmm_free_page((void *)paddr);
    }
    pmap_flush();
  }

  p->brk = brk;
  return brk;
}
