#include <i386/endian.h>
#include <netinet/udp.h>
#include <stdio.h>

void udp_input(const struct ip *ip, const struct udphdr *udp) {
  (void)ip;

  printf("UDP: src_port=%u dst_port=%u length=%u checksum=%u\n", ntohs(udp->src_port), ntohs(udp->dst_port),
         ntohs(udp->length), ntohs(udp->checksum));
  unsigned int data_len = ntohs(udp->length) - sizeof(struct udphdr);
  char *data = (char *)(udp + 1);
  data[data_len] = 0;
  printf("UDP: data: '%s'\n", data);
}
