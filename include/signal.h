#ifndef __INCLUDE_SIGNAL_H__
#define __INCLUDE_SIGNAL_H__

#include <sys/types.h>

#define SIGHUP 1   // hangup
#define SIGINT 2   // interrupt (ctrl-c)
#define SIGKILL 9  // kill process
#define SIGSEGV 11 // segmentation fault
#define SIGTERM 15 // terminate process

typedef void (*sighandler_t)(int);

#define SIG_ERR ((sighandler_t) - 1)
#define SIG_DFL ((sighandler_t)0)
#define SIG_IGN ((sighandler_t)1)

int kill(pid_t pid, int sig);
int raise(int sig);

sighandler_t signal(int sig, sighandler_t handler);

#endif
