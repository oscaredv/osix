#ifndef __INCLUDE_DIRENT_H__
#define __INCLUDE_DIRENT_H__

#define DIRENT_NAME_SIZE 30

struct dirent {
  unsigned short d_ino;
  char d_name[DIRENT_NAME_SIZE];
};

#endif
