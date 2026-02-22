#ifndef __I386_PMAP_H__
#define __I386_PMAP_H__

#include <stdint.h>

struct pmap {
  void *pg_dir_paddr;
};

#endif
