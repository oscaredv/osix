#ifndef __SYS_PROC_H__
#define __SYS_PROC_H__

#include <i386/idt.h>
#include <i386/pmap.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/types.h>

// Process states
enum procstate { STATE_NULL, STATE_IDLE, STATE_RUNNING, STATE_SLEEP, STATE_ZOMBIE };

struct interrupt_frame;

struct proc {
  enum procstate state;
  pid_t pid;
  pid_t pgrp;
  struct proc *parent;
  void *esp0;
  void *kstack;
  unsigned long brk;
  struct pmap pmap;
  struct interrupt_frame *context;
  int exit_status;
  void *wchan;
  struct inode *cwd;
  struct file *ofile[NFILE];

  uid_t uid;
  gid_t gid;
  int ngroups;
  gid_t groups[NGROUPS];

  struct sigacts sigact;
  unsigned int sig_pending;
  unsigned int sig_mask;
};

#define NPROC 8

extern struct proc proc[NPROC];

extern struct proc *cur_proc;

pid_t wait(int *status);

void sleep(void *wchan);
void wakeup(void *wchan);

struct proc *proc_alloc();

void proc_init();
void proc_create(const char *path);
void proc_init_kstack(struct proc *p);
uintptr_t proc_init_ustack(uintptr_t ustack, const char *argv[], const char *parent_envp[]);
void proc_init_user_context(struct proc *p, uintptr_t user_esp);
unsigned long proc_break(struct proc *p, unsigned long brk);
void proc_exit(struct proc *p, int status);

#endif
