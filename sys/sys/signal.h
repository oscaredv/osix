#ifndef __SYS_SIGNAL_H__
#define __SYS_SIGNAL_H__

#include <signal.h>

#define NSIG 16

struct sigacts {
  sighandler_t handlers[NSIG];
  struct interrupt_frame context;
  void *dispatcher;
};

struct proc;

int killproc(struct proc *p, int sig);

sighandler_t kern_signal(int, sighandler_t, void *sig_dispatcher);

void issignal(struct proc *p);

int signal_pending(struct proc *proc);

void sigexit(struct proc *p, int sig);

void sigreturn(int sig);

#endif
