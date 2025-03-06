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
  asm volatile("int $0x80" : : "a"(SYS_sigreturn), "b"(sig));
}

// Syscall wrapper for installing signal handler
sighandler_t signal(int sig, sighandler_t handler) {
  void *old_handler;
  asm volatile("int $0x80" : "=a"(old_handler) : "0"(SYS_signal), "b"(sig), "c"(handler), "d"(sig_handler_dispatch));
  if (old_handler == SIG_ERR) {
    errno = EINVAL;
  }
  return old_handler;
}

int kill(pid_t pid, int sig) {
  int ret;
  asm volatile("int $0x80" : "=a"(ret) : "0"(SYS_kill), "b"(pid), "c"(sig));
  if (ret < 0) {
    errno = -ret;
    ret = -1;
  }
  return ret;
}

int raise(int sig) { return kill(getpid(), sig); }
