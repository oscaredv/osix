#ifndef __DEV_PCI_H__
#define __DEV_PCI_H__

#include <stdint.h>

#define PCI_MAX_DEV 16

struct pci_hdr {
  uint16_t vendor_id;
  uint16_t device_id;
  uint16_t command;
  uint16_t status;
  uint8_t rev_id;
  uint8_t prog_if;
  uint8_t sub_class_id;
  uint8_t class_id;
  uint8_t cache_line_size;
  uint8_t latency_timer;
  uint8_t header_type;
  uint8_t bist;
  uint32_t bar0;
  uint32_t bar1;
  uint32_t bar2;
  uint32_t bar3;
  uint32_t bar4;
  uint32_t bar5;
  uint32_t cardbus_cis_ptr;
  uint16_t subsys_vendor_id;
  uint16_t subsys_id;
  uint32_t exp_rom_base_addr;
  uint32_t reserved0;
  uint32_t reserved1;
  uint8_t irq;
  uint8_t interrupt_pin;
  uint8_t min_grant;
  uint8_t max_latency;
} __attribute__((packed));

typedef union {
  struct pci_hdr h;
  uint32_t raw[16];
} pci_hdr_t;

typedef struct {
  uint8_t bus;
  uint8_t slot;
  uint8_t function;
  pci_hdr_t hdr;
} pci_dev_t;

extern pci_dev_t pci_dev[];
extern unsigned int pci_num_devs;

void pci_probe(void);

void pci_write_hdr_reg(pci_dev_t *dev, uint8_t offset);

#endif
