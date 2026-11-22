#include <conf/config.h>
#include <errno.h>
#include <i386/idt.h>
#include <i386/io.h>
#include <i386/pic.h>
#include <i386/uart.h>
#include <stdio.h>
#include <sys/proc.h>
#include <sys/signal.h>
#include <sys/tty.h>

#define DATA_REGISTER(port_base) port_base + 0
#define INTERRUPT_ENABLE_REGISTER(port_base) port_base + 1
#define FIFO_CONTROL_REGISTER(port_base) port_base + 2
#define LINE_CONTROL_REGISTER(port_base) port_base + 3
#define MODEM_CONTROL_REGISTER(port_base) port_base + 4
#define LINE_STATUS_REGISTER(port_base) port_base + 5

#define LINE_CONTROL_DATA_BITS_8 3
#define LINE_CONTROL_DATA_BITS_7 2
#define LINE_CONTROL_STOP_BITS_1 0
#define LINE_CONTROL_STOP_BITS_2 4
#define LINE_CONTROL_PARITY_NONE 0

#define FIFO_CONTROL_QUEUE_ENABLE 1
#define FIFO_CONTROL_CLEAR_RX_QUEUE 2
#define FIFO_CONTROL_CLEAR_TX_QUEUE 4
#define FIFO_CONTROL_LENGTH_14 3 << 6

#define MODEM_CONTROL_DTR_ENABLE 1
#define MODEM_CONTROL_RTS_ENABLE 2
#define MODEM_CONTROL_OUT2 8

#define INTERRUPT_ENABLE_DATA_AVAILABLE 1

typedef struct {
  uint16_t port;
  char rx_buffer[256];
  volatile uint8_t rx_head, rx_tail;
  dev_t dev;
} uart_t;

uart_t uart_devsw[SERIAL_PORTS];

void uart_intr(uart_t *uart) {
  unsigned char line_status = inb(LINE_STATUS_REGISTER(uart->port));
  if (line_status & 0x01) {
    char c = inb(DATA_REGISTER(uart->port));

    uart->rx_buffer[uart->rx_head++] = c;
    if (uart->rx_head == uart->rx_tail) {
      uart->rx_tail++;
    }
    wakeup(uart_intr);

    tty_input(uart->dev, c);
  }
}

void uart_intr0() {
  uart_intr(&uart_devsw[0]);
  pic_ack();
}

void uart_intr1() {
  uart_intr(&uart_devsw[1]);
  pic_ack();
}

void uart_set_params(uint16_t port, int divisor) {
  // BAUD rate divisor
  outb(0x80, LINE_CONTROL_REGISTER(port));
  outb(divisor, DATA_REGISTER(port));
  outb(0, INTERRUPT_ENABLE_REGISTER(port));

  // 8/N/1
  uint8_t line_control = LINE_CONTROL_DATA_BITS_8 | LINE_CONTROL_STOP_BITS_1 | LINE_CONTROL_PARITY_NONE;
  outb(line_control, LINE_CONTROL_REGISTER(port));

  uint8_t fifo_control =
      FIFO_CONTROL_QUEUE_ENABLE | FIFO_CONTROL_CLEAR_RX_QUEUE | FIFO_CONTROL_CLEAR_TX_QUEUE | FIFO_CONTROL_LENGTH_14;
  outb(fifo_control, FIFO_CONTROL_REGISTER(port));

  uint8_t modem_control = MODEM_CONTROL_DTR_ENABLE | MODEM_CONTROL_RTS_ENABLE | MODEM_CONTROL_OUT2;
  outb(modem_control, MODEM_CONTROL_REGISTER(port));
}

void serial_console_init() {
  uart_devsw[0].port = 0x3F8;
  uart_set_params(0x3F8, BAUD_RATE_DIVISOR_38400);
  uart_write(0, "\n", 1);
}

void uart_init_port(dev_t dev, uint16_t port, int irq, int divisor, void *isr) {
  uart_t *uart = &uart_devsw[minor(dev)];
  uart->port = port;
  uart->rx_head = 0;
  uart->rx_tail = 0;
  uart->dev = dev;

  // Disable interrupts
  outb(0, INTERRUPT_ENABLE_REGISTER(port));

  uart_set_params(port, divisor);

  // Enable interrupt for data available
  outb(INTERRUPT_ENABLE_DATA_AVAILABLE, INTERRUPT_ENABLE_REGISTER(port));

  printf("ttyS%d io=0x%x, irq=%d, baud=%d\n", minor(dev), port, irq, 115200 / divisor);

  idt_set_isr(irq, isr);
  pic_enable_interrupt(irq);
}

void uart_init() {
#if SERIAL_PORTS > 0
#if SERIAL_CONSOLE == 1
  uart_init_port(makedev(1, 0), 0x3F8, 4, BAUD_RATE_DIVISOR_38400, uart_intr0);
#else
  uart_init_port(makedev(3, 0), 0x3F8, 4, BAUD_RATE_DIVISOR_38400, uart_intr0);
#endif
#endif
#if SERIAL_PORTS > 1
  uart_init_port(makedev(3, 1), 0x2F8, 3, BAUD_RATE_DIVISOR_38400, uart_intr1);
#endif
#if SERIAL_PORTS > 2
  serial_init_port(makedev(3, 2), 0x3E8, 4, BAUD_RATE_DIVISOR_38400, uart_intr0);
#endif
#if SERIAL_PORTS > 3
  serial_init_port(makedev(3, 3), 0x2E8, 3, BAUD_RATE_DIVISOR_38400, uart_intr1);
#endif
}

int uart_getc(struct inode *inode, dev_t dev) {
  if (minor(dev) >= SERIAL_PORTS)
    return -ENODEV;

  uart_t *uart = &uart_devsw[minor(dev)];
  while (uart->rx_head == uart->rx_tail) {
    if (signal_pending(cur_proc))
      return -EINTR;

    iunlock(inode);
    sleep(uart_intr);
    ilock(inode);

    if (signal_pending(cur_proc))
      return -EINTR;
  }

  return uart->rx_buffer[uart->rx_tail++];
}

ssize_t uart_read(struct inode *inode, dev_t dev, void *buf, size_t count) {
  if (minor(dev) >= SERIAL_PORTS || uart_devsw[minor(dev)].port == 0)
    return -ENODEV;

  char *p = (char *)buf;
  for (size_t i = 0; i < count; i++) {
    int c = uart_getc(inode, dev);
    if (c < 0)
      return c;
    *p++ = c;
  }
  return count;
}

ssize_t uart_write(dev_t dev, const void *buf, size_t count) {
  if (minor(dev) >= SERIAL_PORTS || uart_devsw[minor(dev)].port == 0)
    return -ENODEV;

  uart_t *uart = &uart_devsw[minor(dev)];
  for (size_t c = 0; c < count; ++c) {
    while (inb(LINE_STATUS_REGISTER(uart->port)) == 0)
      ;
    outb(((char *)buf)[c], DATA_REGISTER(uart->port));
  }
  return count;
}

int uart_putchar(dev_t dev, int c) {
  if (minor(dev) >= SERIAL_PORTS || uart_devsw[minor(dev)].port == 0)
    return -ENODEV;

  uart_t *uart = &uart_devsw[minor(dev)];

  if (c == '\n')
    uart_putchar(dev, '\r');

  while (inb(LINE_STATUS_REGISTER(uart->port)) == 0)
    ;
  outb(c, DATA_REGISTER(uart->port));

  return c;
}
