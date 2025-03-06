#ifndef __SYS_NETINET_UDP_H__
#define __SYS_NETINET_UDP_H__

#include <netinet/ip.h>
#include <stdint.h>

struct udphdr {
  uint16_t src_port; // source port
  uint16_t dst_port; // destination port
  uint16_t length;   // udp length
  uint16_t checksum; // udp checksum
} __attribute__((packed));

void udp_input(const struct ip *ip, const struct udphdr *udp);

#endif