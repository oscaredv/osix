#include <i386/endian.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <stdio.h>
#include <string.h>

void ip_output(uint32_t dst, uint32_t src, uint8_t ip_proto, const void *phdr, size_t phdr_len) {
  struct ip ip;
  ip.ip_ver = 4;
  ip.ip_hl = 5;
  ip.ip_tos = 0;
  ip.ip_len = 0; // ???
  ip.ip_id = 0;  // incr
  ip.ip_offset = 0;
  ip.ip_ttl = 64;
  ip.ip_proto = ip_proto;
  ip.ip_checksum = 0; // ???
  ip.ip_dst = htonl(dst);
  ip.ip_src = htonl(src);

  uint8_t packet[1024];
  memcpy(packet, &ip, sizeof(struct ip));
  memcpy(packet + sizeof(struct ip), phdr, phdr_len);

  ether_output(ETHERTYPE_IP, 0, packet, sizeof(struct ip) + phdr_len);
}

void ip_input(const struct ip *ip, size_t data_len) {
  printf("RX ip(%d):\n", sizeof(struct ip));
  hexdump(ip, sizeof(struct ip));
  printf("RX data(%d):\n", data_len - sizeof(struct ip));
  hexdump(ip + 1, data_len - sizeof(struct ip));

  //   (void)data;
  //   (void)data_len;

  printf("IP: ver=%d header=%d tos=%d len=%d id=%d offs=%d\n", ip->ip_ver, sizeof(uint32_t) * ip->ip_hl, ip->ip_tos,
         ntohs(ip->ip_len), ntohs(ip->ip_id), ntohs(ip->ip_offset));
  printf("IP: ttl=%d proto=%d chksm=%x src=%x dst=%x\n", ip->ip_ttl, ip->ip_proto, ntohs(ip->ip_checksum),
         ntohl(ip->ip_src), ntohl(ip->ip_dst));
  printf("ip hdr size=%d\n", sizeof(struct ip));

  switch (ip->ip_proto) {
  case IPPROTO_TCP:
    tcp_input(ip, (struct tcphdr *)(ip + 1));
    break;
  case IPPROTO_UDP:
    udp_input(ip, (struct udphdr *)(ip + 1));
    break;
  case IPPROTO_ICMP:
    printf("ICMP!!!\n");
    break;
  default:
    printf("UNKNOWN IP PROTO %d!\n", ip->ip_proto);
    break;
  }
}