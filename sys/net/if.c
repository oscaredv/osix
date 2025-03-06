#include <dev/if_re.h>
#include <i386/endian.h>
#include <net/arp.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <stdio.h>

void ether_input(/*struct ifnet *ifp,*/ struct ether_header *eh, const void *data, size_t data_len) {
  (void)data_len;
  // printf("ether_input:\n");
  // printf("RX: src=%02x:%02x:%02x:%02x:%02x:%02x ", eh->ether_src[0], eh->ether_src[1], eh->ether_src[2],
  //        eh->ether_src[3], eh->ether_src[4], eh->ether_src[5]);
  // printf("dst=%02x:%02x:%02x:%02x:%02x:%02x ", eh->ether_dst[0], eh->ether_dst[1], eh->ether_dst[2],
  // eh->ether_dst[3],
  //        eh->ether_dst[4], eh->ether_dst[5]);
 //printf(" eth_type=0x%04x payload len=%d\n", eh->ether_type, data_len);
  printf("RX eh(%d):\n", sizeof(struct ether_header));
  hexdump(eh, sizeof(struct ether_header));

  switch (ntohs(eh->ether_type)) {
  case ETHERTYPE_ARP:
    arp_input(data);
    break;
  case ETHERTYPE_IP:
    ip_input(data, data_len);
    break;
  }
}

// int ether_output(struct ifnet *ifp, struct mbuf *m0, struct sockaddr *dst, struct rtentry *rt0) { }
int ether_output(uint16_t ether_type, uint32_t dst, const void *data, size_t data_size) {
  (void)dst;
  uint8_t hwaddr[6] = {0x52, 0x55, 0x0a, 0x00, 0x02, 0x02};
  return re_output(ether_type, hwaddr, data, data_size);
}
