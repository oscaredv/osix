#ifndef __SYS_NET_IF_ETHER_H__
#define __SYS_NET_IF_ETHER_H__

#include <stdint.h>

struct ether_header {
  uint8_t ether_dst[6];
  uint8_t ether_src[6];
  uint16_t ether_type;
} __attribute__((packed));

// Ethernet protocols
#define ETHERTYPE_IP 0x0800
#define ETHERTYPE_ARP 0x0806

#endif