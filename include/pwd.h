#ifndef __INCLUDE_PWD_H__
#define __INCLUDE_PWD_H__

struct passwd {
  char *pw_name;
  char *pw_passwd;
  int pw_uid;
  int pw_gid;
  char *pw_gecos;
  char *pw_dir;
  char *pw_shell;
};

struct passwd *getpwuid(short uid);
struct passwd *getpwnam(const char *name);

#endif
