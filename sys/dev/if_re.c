#include <dev/if_re.h>
#include <dev/pci.h>
#include <i386/endian.h>
#include <i386/idt.h>
#include <i386/io.h>
#include <i386/pic.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#define RE_MAX_DEV 2

// #define RTL8139_VENDOR_ID 0x10EC
// #define RTL8139_DEVICE_ID 0x8139
// #define RX_BUF_SIZE 8192
// #define CAPR 0x38
// #define RX_READ_POINTER_MASK (~3)
#define ROK (1 << 0)
// #define RER                 (1<<1)
#define TOK (1 << 2)
// #define TER     (1<<3)
// #define TX_TOK  (1<<15)

typedef struct {
  uint32_t io_base;
  uint16_t irq;
  uint16_t dev_id;
  uint8_t mac[6];
  uint8_t rx_buf[8192 + 16 + 1500];
  uint16_t rx_buf_head;
  uint8_t tx_round_robin;
  uint8_t tx_buf[4][1500];
} re_dev_t;

re_dev_t re_devsw[RE_MAX_DEV];
uint32_t re_num_dev = 0;

typedef struct {
  uint16_t header;
  uint16_t len;
  struct ether_header eh;
  uint8_t payload[1500];
} __attribute__((packed)) re_buf_t;

void re_send(re_dev_t *re, const struct ether_header *eh, const void *data, size_t data_len);

void re_input(re_dev_t *re) {
  re_buf_t *buf = (re_buf_t *)&re->rx_buf[re->rx_buf_head];
  
  size_t frame_len = buf->len;
  size_t data_len = frame_len - sizeof(struct ether_header);

  struct ether_header *eh = &buf->eh;
  printf("recvd %d\n", frame_len);
  hexdump(eh, frame_len);
  ether_input(eh, buf->payload, data_len);

  // switch (ntohs(eh->ether_type)) {
  // case 0x0800:
  //   re_ipv4(re, eh, buf->payload, data_len);
  //   break;
  // // default:
  // //   printf("Unhandled protocol %x\n", eh->ether_type);
  // }
  // printf("\n");

  // Update rx buffer offet
  re->rx_buf_head += (buf->len + 4 + 3) & ~3;
}

int re_output(uint16_t ether_type, const uint8_t *dst, const uint8_t *data, size_t data_len) {
  struct ether_header eh;
  eh.ether_type = htons(ether_type);
  memcpy(eh.ether_dst, dst, 6);
  re_dev_t *re = &re_devsw[0];
  memcpy(eh.ether_src, re->mac, 6);

  re_send(&re_devsw[0], &eh, data, data_len);
  return 0;
}

// typedef struct {
//   uint8_t ihl : 4, version : 4;
//   uint8_t tos;
//   uint16_t total_len;
//   uint16_t ident;
//   uint16_t frag_offset;
//   uint8_t ttl;
//   uint8_t proto;
//   uint16_t checksum;
//   uint32_t src;
//   uint32_t dst;
//   uint8_t payload;
// } __attribute__((packed)) ipv4_t;

// typedef struct {
//   uint16_t src_port; /* source port */
//   uint16_t dst_port; /* destination port */
//   uint16_t length;   /* udp length */
//   uint16_t chksm;    /* udp checksum */
//   uint8_t payload;
// } __attribute__((packed)) udphdr_t;

// void re_tcp(re_dev_t *re, tcphdr_t *t);
// void re_udp(re_dev_t *re, udphdr_t *u);

// void re_ipv4(re_dev_t *re, struct ether_header *eh, uint8_t *data, size_t data_len) {
//   (void)eh;
//   ipv4_t *ip = (ipv4_t *)data;
//   printf("IPv4:\n");
//   hexdump(ip, data_len);
//   printf("ver=%d ihl=%d tos=%d totlen=%d id=%d offs=%d ", ip->version, ip->ihl, ip->tos, ntohs(ip->total_len),
//          ntohs(ip->ident), ntohs(ip->frag_offset));
//   printf("ttl=%d proto=%d chksm=%x src=%x dst=%x\n", ip->ttl, ip->proto, ntohs(ip->checksum), ntohl(ip->src),
//          ntohl(ip->dst));

//   switch (ip->proto) {
//   case 6:
//     re_tcp(re, (tcphdr_t *)&ip->payload);
//     break;
//   case 17:
//     re_udp(re, (udphdr_t *)&ip->payload);
//     break;
//   default:
//     printf("UNKNOWN IP PROTO %d!\n", ip->proto);
//     break;
//   }
// }

// void re_tcp(re_dev_t *re, tcphdr_t *t) {
//   (void)re;
//   printf("TCP:\n");
//   hexdump(t, 44);
//   printf("src_port=%d dst_port=%d seq=%d ack_seq=%d\n", ntohs(t->src_port), ntohs(t->dst_port), ntohl(t->seq),
//          ntohl(t->ack_seq));
//   printf("fin=%d syn=%d rst=%d psh=%d ack=%d urg=%d\n", t->fin, t->syn, t->rst, t->psh, t->ack, t->urg);
//   printf("doff=%d window=%d\n", ntohs(t->doff), ntohs(t->window));
// }

// void re_udp(re_dev_t *re, udphdr_t *u) {
//   (void)re;
//   printf("UDP:\n");
//   printf("src_port=%d dst_port=%d length=%d checksum=%d\n", ntohs(u->src_port), ntohs(u->dst_port), ntohs(u->length),
//          ntohs(u->chksm));
//   size_t payload_len = ntohs(u->length) - sizeof(udphdr_t);

//   hexdump(&u->payload, payload_len);
// }

void re_transmit(re_dev_t *re) { printf("** TX OK 0x%x **\n", re->io_base); }

void re_intr() {
  for (unsigned i = 0; i < re_num_dev; i++) {
    re_dev_t *re = &re_devsw[i];
    uint16_t status = inw(re->io_base + 0x3E);
    if (status & TOK) {
      re_transmit(re);
    }
    if (status & ROK) {
      re_input(re);
    }

    outw(0x05, re->io_base + 0x3E);
  }
  pic2_ack();
}

void re_send(re_dev_t *re, const struct ether_header *eh, const void *data, size_t data_len) {
  // printf("resend(..., %d)\n", data_len);
  //  printf("src %02x:%02x:%02x:%02x:%02x:%02x\n", eh->ether_src[0], eh->ether_src[1], eh->ether_src[2],
  //  eh->ether_src[3],
  //         eh->ether_src[4], eh->ether_src[5]);
  //  printf("dst %02x:%02x:%02x:%02x:%02x:%02x\n", eh->ether_dst[0], eh->ether_dst[1], eh->ether_dst[2],
  //  eh->ether_dst[3],
  //         eh->ether_dst[4], eh->ether_dst[5]);
  uint8_t *dst = re->tx_buf[re->tx_round_robin];
  memcpy(dst, eh, sizeof(struct ether_header));
  memcpy(dst + sizeof(struct ether_header), data, data_len);

  // hexdump(re->tx_buf[re->tx_round_robin], data_len + sizeof(struct ether_header));

  //   uint32_t status =   inl(re->io_base + 0x10);
  //  int own = status & (1<<13);
  //  int txok = status & (1<<15);
  //   printf("re tx[0] status=%x own=%x\n", status, own, txok);

  outl((uint32_t)&re->tx_buf[re->tx_round_robin], re->io_base + 0x20 + (re->tx_round_robin << 2));
  outl(data_len + sizeof(struct ether_header), re->io_base + 0x10 + (re->tx_round_robin << 2));
  re->tx_round_robin = (re->tx_round_robin + 1) & 3;
}

void re_init(unsigned dev_no, pci_dev_t *pci) {
  re_dev_t *re = &re_devsw[dev_no];
  memset(re, 0, sizeof(re_dev_t));

  re->io_base = pci->hdr.h.bar0 & 0xFFFFFFFC;
  re->irq = pci->hdr.h.irq;
  re->dev_id = pci->hdr.h.device_id;
  uint16_t io = re->io_base;
  for (int i = 0; i < 6; i++) {
    re->mac[i] = inb(io++);
  }
  printf("re%u: Realtek %X io=0x%x irq=%d mac=%02x:%02x:%02x:%02x:%02x:%02x\n", dev_no, re->dev_id, re->io_base,
         re->irq, re->mac[0], re->mac[1], re->mac[2], re->mac[3], re->mac[4], re->mac[5]);

  // Enable PCI bus mastering by setting bit 2 in the PCI command field
  pci->hdr.h.command |= 4;
  pci_write_hdr_reg(pci, 4); // Command is at offset 4

  // Power on the chip
  outb(0, re->io_base + 0x52);

  // Soft reset device, and wait until done.
  outb(0x10, re->io_base + 0x37);
  while ((inb(re->io_base + 0x37) & 0x10) != 0)
    ;

  // Receive buffer
  outl((uint32_t)&re->rx_buf, re->io_base + 0x30);
  outl(0x0F | (1 << 7), re->io_base + 0x44); // (1 << 7) is the WRAP bit, 0x0F is AB+AM+APM+AAP

  // Interrupt mask reg - enable Transmit OK (TOK) and Receive OK (ROK)
  outw(0x0005, re->io_base + 0x3C);

  // Enable RX and TX functions
  outb(0x0C, re->io_base + 0x37); // Sets the RE and TE bits high

  idt_set_isr(re->irq, re_intr);
  pic_enable_interrupt(re->irq);
}

void re_probe(void) {
  for (unsigned i = 0; i < pci_num_devs && re_num_dev < RE_MAX_DEV; i++) {
    if (pci_dev[i].hdr.h.vendor_id == 0x10EC) {
      if (pci_dev[i].hdr.h.device_id == 0x8139) {
        re_init(re_num_dev++, &pci_dev[i]);
      }
    }
  }
}
