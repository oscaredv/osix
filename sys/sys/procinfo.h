#ifndef __SYS_PROCINFO_H__
#define __SYS_PROCINFO_H__

#include <stdlib.h>
#include <sys/types.h>

#define PROC_NAME_LEN 16

struct procinfo {
  pid_t pid;
  pid_t ppid;
  enum procstate state;
  char name[PROC_NAME_LEN];
};

int getprocs(struct procinfo *buf, size_t *len);

#endif
