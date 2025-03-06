#ifndef __I386_ENDIAN_H__
#define __I386_ENDIAN_H__

#include <stdint.h>

#define LITTLE_ENDIAN 1234
#define BIG_ENDIAN 4321

#define BYTE_ORDER LITTLE_ENDIAN

static inline uint16_t bswap16(uint16_t v) { return ((v >> 8) & 0xFF) | ((v & 0xFF) << 8); }

static inline uint32_t bswap32(uint32_t v) {
  asm volatile("bswap %0" : "=r"(v) : "0"(v));
  return v;
}

#if BYTE_ORDER == BIG_ENDIAN
#define ntohl(x) x
#define ntohs(x) x
#define htonl(x) x
#define htons(x) x
#else
#define ntohl(x) bswap32(x)
#define ntohs(x) bswap16(x)
#define htonl(x) bswap32(x)
#define htons(x) bswap16(x)
#endif

#endif
