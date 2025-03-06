#include <dev/pci.h>
#include <i386/io.h>
#include <stdio.h>
#include <string.h>

pci_dev_t pci_dev[PCI_MAX_DEV];
unsigned int pci_num_devs = 0;

uint32_t pci_address(uint32_t bus, uint32_t slot, uint32_t func, uint8_t offset) {
  return (bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000U;
}

void pci_probe() {
  pci_num_devs = 0;
  memset(pci_dev, 0, sizeof(pci_dev));

  for (uint32_t bus = 0; bus < 256; bus++) {
    for (uint32_t slot = 0; slot < 32; slot++) {
      for (uint32_t func = 0; func < 8; func++) {
        uint32_t address = pci_address(bus, slot, func, 0);
        pci_dev_t *dev = &pci_dev[pci_num_devs];

        for (int i = 0; i < 16; i++) {
          outl(address, 0xCF8);
          dev->hdr.raw[i] = inl(0x0CFC);
          address += 4;
          if (dev->hdr.h.vendor_id == 0xFFFF)
            break;
        }

        if (dev->hdr.h.vendor_id != 0xFFFF) {
          dev->bus = bus;
          dev->slot = slot;
          dev->function = func;
          // printf("pci bus=%d slot=%d function=%d vendor: 0x%x device: 0x%x irq=%d\n", bus, slot, func,
          //        dev->hdr.h.vendor_id, dev->hdr.h.device_id, dev->hdr.h.irq);
          pci_num_devs++;
        }
      }
    }
  }
}

void pci_write_hdr_reg(pci_dev_t *dev, uint8_t offset) {
  outl(pci_address(dev->bus, dev->slot, dev->function, offset), 0xCF8);
  outl(dev->hdr.raw[offset >> 2], 0x0CFC);
  //printf("PCI write bus %d slot %d func %d offset %d index %d\n", dev->bus, dev->slot, dev->function, offset, (offset>>2));
}
