#ifndef __DEV_IF_RE_H__
#define __DEV_IF_RE_H__

#include <stdint.h>
#include <stddef.h>

void re_probe(void);

int re_output(uint16_t ether_type, const uint8_t *dst, const uint8_t *data, size_t data_len);

#endif
