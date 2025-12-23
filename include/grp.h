#ifndef __INCLUDE_GRP_H__
#define __INCLUDE_GRP_H__

#include <sys/types.h>

#define NGROUPMEMB 16

struct group {
  char *gr_name;
  char *gr_passwd;
  gid_t gr_gid;
  char *gr_mem[NGROUPMEMB];
};

struct group *getgrgid(short gid);

int initgroups(const char *name, gid_t basegid);
int getgrouplist(const char *name, gid_t basegid, gid_t *groups, int *ngroups);
int setgroups(int ngroups, const gid_t *groups);

#endif
