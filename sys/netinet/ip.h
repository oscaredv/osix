#ifndef __SYS_NET_IP_H__
#define __SYS_NET_IP_H__

#include <stddef.h>
#include <stdint.h>

// TODO: move to netinet ?

#define IPPROTO_ICMP 1
#define IPPROTO_TCP 6
#define IPPROTO_UDP 17

#define IP_DF 0x4000      // dont fragment flag
#define IP_MF 0x2000      // more fragments flag
#define IP_OFFMASK 0x1FFF // mask for fragmenting bits

struct ip {
  uint8_t ip_hl : 4, ip_ver : 4;
  uint8_t ip_tos;
  uint16_t ip_len;
  uint16_t ip_id;
  uint16_t ip_offset;
  uint8_t ip_ttl;
  uint8_t ip_proto;
  uint16_t ip_checksum;
  uint32_t ip_src;
  uint32_t ip_dst;
} __attribute__((packed));

void ip_input(const struct ip *ip, size_t data_len);
void ip_output(uint32_t dst, uint32_t src, uint8_t ip_proto, const void *phdr, size_t phdr_len);

#endif