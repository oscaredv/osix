#include <i386/endian.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>

void tcp_output(uint32_t dst, uint32_t src, const struct tcphdr *tcp) {
  //
  ip_output(dst, src, IPPROTO_TCP, tcp, sizeof(struct tcphdr));
}

void tcp_input(const struct ip *ip, const struct tcphdr *tcp) {
  (void)ip;

  printf("TCP: src_port=%d dst_port=%d seq=%d ack_seq=%d\n", ntohs(tcp->src_port), ntohs(tcp->dst_port),
         ntohl(tcp->seq), ntohl(tcp->ack_seq));
  printf("TCP: fin=%d syn=%d rst=%d psh=%d ack=%d urg=%d\n", tcp->fin, tcp->syn, tcp->rst, tcp->psh, tcp->ack,
         tcp->urg);
  printf("TCP: doff=%d window=%d\n", ntohs(tcp->doff), ntohs(tcp->window));

  if (tcp->syn == 1 && tcp->ack == 0) {
    printf("SYN - handshake start - send SYN-ACK\n");
    struct tcphdr r;
    memset(&r, 0, sizeof(struct tcphdr));
    r.syn = 1;
    r.ack = 1;
    r.ack_seq = htonl(ntohl(tcp->seq) + 1);
    r.seq = htonl(100);
    r.src_port = tcp->dst_port;
    r.dst_port = tcp->src_port;
    tcp_output(ntohl(ip->ip_src), ntohl(ip->ip_dst), &r);
  } else if (tcp->syn == 0 && tcp->ack == 1) {

  } else {
  }
  // SYN, SYN-ACK, ACK
}
