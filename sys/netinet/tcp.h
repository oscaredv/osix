#ifndef __SYS_NETINET_TCP_H__
#define __SYS_NETINET_TCP_H__

#include <netinet/ip.h>
#include <stdint.h>

struct tcphdr {
  uint16_t src_port;
  uint16_t dst_port;
  uint32_t seq;
  uint32_t ack_seq;
  uint16_t res1 : 4;
  uint16_t doff : 4; // header length?
  uint16_t fin : 1;
  uint16_t syn : 1;
  uint16_t rst : 1;
  uint16_t psh : 1;
  uint16_t ack : 1;
  uint16_t urg : 1;
  uint16_t res2 : 2;
  uint16_t window;
  uint16_t check;
  uint16_t urg_ptr;
} __attribute__((packed));

void tcp_input(const struct ip *ip, const struct tcphdr *tcp);

#endif