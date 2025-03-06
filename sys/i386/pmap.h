#ifndef __I386_PMAP_H__
#define __I386_PMAP_H__

#include <stdint.h>

struct pmap {
  uint32_t *pg_dir;
  void *pg_dir_paddr;
};

#endif
