#ifndef __I386_UART_H__
#define __I386_UART_H__

#include <stdint.h>
#include <sys/inode.h>
#include <sys/types.h>

#define BAUD_RATE_DIVISOR_115200 1
#define BAUD_RATE_DIVISOR_57600 2
#define BAUD_RATE_DIVISOR_38400 3
#define BAUD_RATE_DIVISOR_19200 6
#define BAUD_RATE_DIVISOR_9600 12
#define BAUD_RATE_DIVISOR_4800 24
#define BAUD_RATE_DIVISOR_2400 48
#define BAUD_RATE_DIVISOR_1200 96

void uart_init();
void serial_console_init();

ssize_t uart_read(struct inode *inode, dev_t dev, void *buf, size_t count);
ssize_t uart_write(dev_t dev, const void *buf, size_t count);

int uart_putchar(dev_t dev, int c);

#endif
