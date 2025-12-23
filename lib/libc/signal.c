#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

void sig_handler_dispatch() {
  int sig;
  void (*sig_handler)(int);

  // Called from the kernel, arguments are in ecx and edx
  // Signal number and signal handler function pointer
  asm("movl %%ecx, %0" : "=r"(sig));
  asm("movl %%edx, %0" : "=r"(sig_handler));

  // Call signal handler
  sig_handler(sig);

  // Return back to kernel
  syscall(SYS_sigreturn, sig);
}

// Syscall wrapper for installing signal handler
sighandler_t signal(int sig, sighandler_t handler) {
  return (sighandler_t)syscall(SYS_signal, sig, (long)handler, (long)sig_handler_dispatch);
}

int kill(pid_t pid, int sig) { return syscall(SYS_kill, pid, sig); }

int raise(int sig) { return kill(getpid(), sig); }
