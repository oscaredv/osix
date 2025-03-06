#include <stddef.h>
#include <sys/proc.h>

pid_t getpid(void) { return cur_proc->pid; }

pid_t getppid(void) {
  if (cur_proc->parent == NULL)
    return 0;
  return cur_proc->parent->pid;
}
