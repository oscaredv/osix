#ifndef __I386_PIT_H__
#define __I386_PIT_H__

#include <time.h>

// Frequency for PIT interrupt. Lowest supported frequency is 18Hz
#define PIT_FREQ 20

extern unsigned int pit_ticks;
extern time_t pit_time;

void pit_init(void);

void pit_settime(time_t time);

time_t pit_gettime(void);

#endif
