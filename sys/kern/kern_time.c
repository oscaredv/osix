#include <i386/pit.h>
#include <stddef.h>
#include <time.h>

time_t time(time_t *t) {
  if (t != NULL) {
    *t = pit_time;
  }
  return pit_time;
}
