#ifndef __INCLUDE_UTIME_H__
#define __INCLUDE_UTIME_H__

#include <time.h>

struct utimbuf {
  time_t actime;  // Access time
  time_t modtime; // Modification time
};

int utime(const char *filename, const struct utimbuf *times);

#endif