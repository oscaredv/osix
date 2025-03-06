#ifndef __SYS_SYSTEM_H__
#define __SYS_SYSTEM_H__

#include <sys/proc.h>

extern char version[];

void yield(void);

void sched(void);

void swtch(void **old_kstack, void *kstack);

void panic(const char *msg);

void buf_init(void);

void inode_init(void);

unsigned int sys_sleep(unsigned int seconds);

#endif