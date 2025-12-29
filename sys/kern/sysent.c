#include <fcntl.h>
#include <i386/idt.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/system.h>
#include <time.h>
#include <unistd.h>

int sys_open(const char *filename, int flags, int mode);
int sys_exec(const char *pathname, const char *argv[], const char *env[]);
int sys_unlink(const char *filepath);
int sys_setgroups(int ngroups, const gid_t *gidset);
int sys_getgroups(int size, gid_t list[]);
int sys_pipe(int fd[2]);

void sysent(struct interrupt_frame *r) {
  cur_proc->context = r;

  switch (r->eax) {
  case SYS_write:
    r->eax = write(r->ebx, (const void *)r->ecx, r->edx);
    break;
  case SYS_read:
    r->eax = read(r->ebx, (void *)r->ecx, r->edx);
    break;
  case SYS_getpid:
    r->eax = getpid();
    break;
  case SYS_fork:
    r->eax = fork();
    break;
  case SYS_getppid:
    r->eax = getppid();
    break;
  case SYS_exit:
    exit(r->ebx);
    break;
  case SYS_wait:
    r->eax = wait((int *)r->ebx);
    break;
  case SYS_exec:
    r->eax = sys_exec((const char *)r->ebx, (const char **)r->ecx, (const char **)r->edx);
    break;
  case SYS_sleep:
    r->eax = sys_sleep(r->ebx);
    break;
  case SYS_time:
    r->eax = time((time_t *)r->ebx);
    break;
  case SYS_brk:
    r->eax = proc_break(cur_proc, r->ebx);
    break;
  case SYS_open:
    r->eax = sys_open((const char *)r->ebx, r->ecx, r->edx);
    break;
  case SYS_close:
    r->eax = close(r->ebx);
    break;
  case SYS_dup:
    r->eax = dup(r->ebx);
    break;
  case SYS_dup2:
    r->eax = dup2(r->ebx, r->ecx);
    break;
  case SYS_chdir:
    r->eax = chdir((const char *)r->ebx);
    break;
  case SYS_stat:
    r->eax = stat((const char *)r->ebx, (struct stat *)r->ecx);
    break;
  case SYS_fstat:
    r->eax = fstat(r->ebx, (struct stat *)r->ecx);
    break;
  case SYS_kill:
    r->eax = kill(r->ebx, r->ecx);
    break;
  case SYS_signal:
    r->eax = (uint32_t)kern_signal(r->ebx, (void *)r->ecx, (void *)r->edx);
    break;
  case SYS_sigreturn:
    sigreturn(r->ebx);
    break;
  case SYS_ioctl:
    r->eax = ioctl(r->ebx, r->ecx, (void *)r->edx);
    break;
  case SYS_lseek:
    r->eax = lseek(r->ebx, r->ecx, r->edx);
    break;
  case SYS_unlink:
    r->eax = sys_unlink((const char *)r->ebx);
    break;
  case SYS_getuid:
    r->eax = getuid();
    break;
  case SYS_getgid:
    r->eax = getgid();
    break;
  case SYS_setuid:
    r->eax = setuid(r->ebx);
    break;
  case SYS_setgid:
    r->eax = setgid(r->ebx);
    break;
  case SYS_setgroups:
    r->eax = sys_setgroups(r->ebx, (const gid_t *)r->ecx);
    break;
  case SYS_getgroups:
    r->eax = sys_getgroups(r->ebx, (gid_t *)r->ecx);
    break;
  case SYS_pipe:
    r->eax = sys_pipe((int *)r->ebx);
    break;
  default:
    panic("Unhandled syscall!");
    break;
  }

  // Before leaving kernel mode, check if we should execute a signal handler
  if (signal_pending(cur_proc)) {
    issignal(cur_proc);
  }
}
