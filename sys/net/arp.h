#ifndef __SYS_NET_ARP_H__
#define __SYS_NET_ARP_H__

#include <net/if_ether.h>
#include <stdint.h>

#define ARP_OP_REQUEST 1
#define ARP_OP_REPLY 2

#define ARP_HTYPE_ETHER 1

struct arphdr {
  uint16_t ar_htype;
  uint16_t ar_ptype;
  uint8_t ar_hlen;   // Hardware address length
  uint8_t ar_plen;   // IP address length
  uint16_t ar_op;    // ARP operation
  uint8_t ar_sha[6]; // sender MAC
  uint32_t ar_spa;   // sender IP
  uint8_t ar_tha[6]; // target MAC
  uint32_t ar_tpa;   // target IP
} __attribute__((packed));

void arp_input(const struct arphdr *arp);

#endif