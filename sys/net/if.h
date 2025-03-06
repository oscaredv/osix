#ifndef __SYS_NET_IF_H__
#define __SYS_NET_IF_H__

#include <net/if_ether.h>
#include <stddef.h>

// TODO: Implement mbufs
// void ether_input(struct ifnet *ifp, struct ether_header *eh, struct mbuf *m);
void ether_input(/*struct ifnet *ifp,*/ struct ether_header *eh, const void *data, size_t data_len);

int ether_output(uint16_t ether_type, uint32_t dst, const void *data, size_t data_size);

#endif