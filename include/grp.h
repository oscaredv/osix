#ifndef __INCLUDE_GRP_H__
#define __INCLUDE_GRP_H__

struct group {
  char *gr_name;
  char *gr_passwd;
  int gr_gid;
  char **gr_mem;
};

struct group *getgrgid(short gid);

#endif
