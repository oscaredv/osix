#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <sys/proc.h>
#include <sys/system.h>
#include <sys/types.h>

int killproc(struct proc *p, int sig) {
  if (p == NULL)
    return -EINVAL;

  p->sig_pending |= 1 << (sig - 1);
  return 0;
}

int killpgrp(short pgrp, int sig) {
  int ret = -ESRCH;
  for (int p = NPROC - 1; p >= 0; p--) {
    if (proc[p].state != STATE_NULL && proc[p].state != STATE_ZOMBIE && proc[p].pgrp == pgrp) {
      if (killproc(&proc[p], sig) == 0)
        ret = 0;
    }
  }
  return ret;
}

int kill(pid_t pid, int sig) {
  if (sig < SIGHUP || sig > SIGTERM)
    return -EINVAL;

  // TODO: pid == 0
  // TODO: pid == -1
  if (pid < -1) {
    // Kill all processes in process group
    return killpgrp(-pid, sig);
  }

  // Kill the process with matching pid
  for (int p = 0; p < NPROC; p++) {
    if (proc[p].state != STATE_NULL && proc[p].state != STATE_ZOMBIE && proc[p].pid == pid) {
      return killproc(&proc[p], sig);
    }
  }
  return -ESRCH;
}

sighandler_t kern_signal(int sig, sighandler_t handler, void *sig_dispatcher) {
  if (sig == SIGKILL || sig < SIGHUP || sig > SIGTERM)
    return (sighandler_t)-EINVAL;

  void *old = cur_proc->sigact.handlers[sig - 1];
  cur_proc->sigact.handlers[sig - 1] = handler;
  cur_proc->sigact.dispatcher = sig_dispatcher;
  return old;
}

void sigexit(struct proc *p, int sig) {
  // TODO: core file?
  proc_exit(p, sig);
}

void postsig(struct proc *p, int sig) {
  if (p->sigact.handlers[sig - 1] == SIG_IGN) {
    // Ignore signal
  } else if (p->sigact.handlers[sig - 1] == SIG_DFL) {
    // TODO: if(sig != SIGCHLD) - Exit except for signals with default action SIG_IGN
    sigexit(p, sig);
  } else {
    // Save process contex, it will be restored in the sigreturn() syscall
    p->sigact.context = *p->context;

    // The next time this process returns to user mode it will execute the signal dispatcher
    p->context->eip = (uint32_t)p->sigact.dispatcher;

    // Send signal number and handler passed to signal dispatcher in registers
    p->context->ecx = sig;
    p->context->edx = (uint32_t)p->sigact.handlers[sig - 1];

    // Mask the signal because signal handlers are not re-entrant
    p->sig_mask |= 1 << (sig - 1);

    // Mark signal as no longer pending
    p->sig_pending &= ~(1 << (sig - 1));

    // If the process was sleeping, wake it up!
    p->state = STATE_RUNNING;
  }
}

void issignal(struct proc *p) {
  for (int sig = 1; sig < NSIG; sig++) {
    unsigned int bit = 1 << (sig - 1);
    if ((p->sig_pending & ~p->sig_mask) & bit) {
      // Take action for signal
      postsig(p, sig);
      break;
    }
  }
}

int signal_pending(struct proc *p) { return (p->sig_pending & ~p->sig_mask) != 0; }

void sigreturn(int sig) {
  // Clear signal bit from signal mask, to allow this signal to be posted again
  cur_proc->sig_mask &= ~(1 << (sig - 1));

  // Restore process context
  *cur_proc->context = cur_proc->sigact.context;
}
