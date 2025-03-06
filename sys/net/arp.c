#include <i386/endian.h>
#include <net/arp.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>

void arp_output(uint16_t op, const uint8_t *sender_hwaddr, uint32_t sender_ip, const uint8_t *target_hwaddr,
                uint32_t target_ip) {
  struct arphdr arp;
  arp.ar_htype = htons(ARP_HTYPE_ETHER);
  arp.ar_ptype = htons(0x0800);
  arp.ar_hlen = 6;
  arp.ar_plen = 4;
  arp.ar_op = htons(op);

  memcpy(arp.ar_sha, sender_hwaddr, 6);
  arp.ar_spa = sender_ip;
  memcpy(arp.ar_tha, target_hwaddr, 6);
  arp.ar_tpa = target_ip;

  ether_output(ETHERTYPE_ARP, sender_ip, &arp, sizeof(arp));
}

void arp_input(const struct arphdr *arp) {
  // printf("ARP: htype=%x ptype=%x hlen=%u plen=%u oper=%x ", ntohs(arp->ar_htype), ntohs(arp->ar_ptype), arp->ar_hlen,
  //        arp->ar_plen, ntohs(arp->ar_op));

  // const uint8_t *sha = arp->ar_sha;
  // printf("src %x %02x:%02x:%02x:%02x:%02x:%02x\n", ntohl(arp->ar_spa), sha[0], sha[1], sha[2], sha[3], sha[4],
  // sha[5]); const uint8_t *tha = arp->ar_tha; printf("dst %x %02x:%02x:%02x:%02x:%02x:%02x\n", ntohl(arp->ar_tpa),
  // tha[0], tha[1], tha[2], tha[3], tha[4], tha[5]);

  uint8_t myhw_addr[] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56}; // TODO: loop through interfaces...

  switch (ntohs(arp->ar_op)) {
  case ARP_OP_REQUEST:
    arp_output(ARP_OP_REPLY, myhw_addr, arp->ar_tpa, arp->ar_sha, arp->ar_spa);
    break;
  case ARP_OP_REPLY:
    printf("reply - add to arp cache\n");
    break;
  }
}
