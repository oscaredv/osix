#ifndef __SYS_TYPES_H__
#define __SYS_TYPES_H__

#define major(x) ((x >> 8) & 0xFF)
#define minor(x) (x & 0xFF)
#define makedev(maj, min) (maj << 8 | min)

typedef short dev_t;
typedef short pid_t;

#endif
