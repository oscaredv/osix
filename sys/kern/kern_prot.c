#include <errno.h>
#include <stddef.h>
#include <sys/proc.h>
#include <sys/types.h>

pid_t getpid(void) { return cur_proc->pid; }

pid_t getppid(void) {
  if (cur_proc->parent == NULL)
    return 0;
  return cur_proc->parent->pid;
}

uid_t getuid(void) { return cur_proc->uid; }

gid_t getgid(void) { return cur_proc->gid; }

int setuid(uid_t uid) {
  if (cur_proc->uid != 0)
    return -EPERM;

  cur_proc->uid = uid;
  return 0;
}

int setgid(gid_t gid) {
  if (cur_proc->uid != 0)
    return -EPERM;

  cur_proc->gid = gid;
  return 0;
}

int sys_setgroups(int ngroups, const gid_t *groups) {
  if (cur_proc->uid != 0)
    return -EPERM;

  cur_proc->ngroups = 0;
  if (groups != NULL && ngroups > 0 && ngroups <= NGROUPS) {
    for (int g = 0; g < ngroups; g++) {
      cur_proc->groups[g] = groups[g];
    }
    cur_proc->ngroups = ngroups;
  }
  return 0;
}

int sys_getgroups(int size, gid_t list[]) {
  if (size == 0)
    return cur_proc->ngroups;

  int ngroups = MIN(size, cur_proc->ngroups);
  if (ngroups < 0)
    return -EINVAL;

  for (int i = 0; i < ngroups; i++) {
    list[i] = cur_proc->groups[i];
  }
  return ngroups;
}
