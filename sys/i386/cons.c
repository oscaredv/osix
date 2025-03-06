#include <conf/config.h>
#include <ctype.h>
#include <i386/cons.h>
#include <i386/io.h>
#include <i386/kb.h>
#include <i386/param.h>
#include <i386/uart.h>
#include <stdio.h>
#include <string.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

enum vga_color {
  VGA_COLOR_BLACK,
  VGA_COLOR_BLUE,
  VGA_COLOR_GREEN,
  VGA_COLOR_CYAN,
  VGA_COLOR_RED,
  VGA_COLOR_MAGENTA,
  VGA_COLOR_BROWN,
  VGA_COLOR_LIGHT_GREY,
  VGA_COLOR_DARK_GREY,
  VGA_COLOR_LIGHT_BLUE,
  VGA_COLOR_LIGHT_GREEN,
  VGA_COLOR_LIGHT_CYAN,
  VGA_COLOR_LIGHT_RED,
  VGA_COLOR_LIGHT_MAGENTA,
  VGA_COLOR_LIGHT_BROWN,
  VGA_COLOR_WHITE
};

uint16_t cn_color;
uint16_t cn_cursor_pos;
uint16_t *cn_buffer;

static inline void cn_update_cursor() {
  outb(0x0F, 0x3D4);
  outb((uint8_t)(cn_cursor_pos & 0xFF), 0x3D5);
  outb(0x0E, 0x3D4);
  outb((uint8_t)((cn_cursor_pos >> 8) & 0xFF), 0x3D5);
}

void cn_init(void) {
#if SERIAL_CONSOLE == 1
  serial_console_init();
#else
  cn_color = (VGA_COLOR_LIGHT_GREEN << 8) | (VGA_COLOR_BLACK << 12);
  cn_buffer = (uint16_t *)P2V(0xB8000);

  // Read cursor position and set console row accordingly
  outb(0x0F, 0x3D4);
  cn_cursor_pos = inb(0x3D5);
  outb(0x0E, 0x3D4);
  cn_cursor_pos |= ((uint16_t)inb(0x3D5)) << 8;
#endif
}

int cn_putchar_raw(int c) {
  if (c == '\r') {
    cn_cursor_pos = (cn_cursor_pos / VGA_WIDTH) * VGA_WIDTH;
  } else if (c == '\n') {
    cn_cursor_pos = ((cn_cursor_pos / VGA_WIDTH) * VGA_WIDTH) + VGA_WIDTH;
  } else if (c == '\b') {
    --cn_cursor_pos;
  } else if (c == '\t') {
    cn_cursor_pos = (cn_cursor_pos + 8) & ~(3);
  } else {
    // Write char to screen
    cn_buffer[cn_cursor_pos++] = c | cn_color;
  }

  if (cn_cursor_pos >= VGA_WIDTH * (VGA_HEIGHT)) {
    // scroll up
    memmove(cn_buffer, &cn_buffer[VGA_WIDTH], VGA_WIDTH * 2 * (VGA_HEIGHT - 1));
    cn_cursor_pos = VGA_WIDTH * (VGA_HEIGHT - 1);
    // Clear last line
    for (size_t c = cn_cursor_pos; c < VGA_WIDTH * VGA_HEIGHT; c++) {
      cn_buffer[c] = ' ';
    }
  }

  // Move cursor
  cn_update_cursor();

  return 0;
}

void cn_erase_line() {
  uint16_t start = (cn_cursor_pos / VGA_WIDTH) * VGA_WIDTH;
  for (uint16_t x = 0; x < VGA_WIDTH; x++) {
    cn_buffer[x + start] &= 0xFF00;
  }
}

void cn_goto(unsigned int x, unsigned int y) {
  x = x % VGA_WIDTH;
  y = (y - 1) % VGA_HEIGHT;
  cn_cursor_pos = y * VGA_WIDTH + x;
  cn_update_cursor();
}

enum esc_state { ESC_NORMAL, ESC_START, ESC_BRACKET };

int cn_putchar(dev_t dev, int c) {
#if SERIAL_CONSOLE == 1
  return uart_putchar(dev, c);
#else
  (void)dev;
  static enum esc_state state = ESC_NORMAL;
  static int args_count = 0;
  static int args[4];

  switch (state) {
  case ESC_NORMAL:
    if (c == '\x1b') {
      state = ESC_START;
      args_count = 0;
      memset(args, 0, sizeof(args));
    } else {
      cn_putchar_raw(c);
    }
    break;
  case ESC_START:
    if (c == '[') {
      state = ESC_BRACKET;
    } else {
      state = ESC_NORMAL;
      cn_putchar_raw('\x1b');
      cn_putchar_raw(c);
    }
    break;
  case ESC_BRACKET:
    if (isprint(c)) {
      if (isdigit(c)) {
        if (args_count == 0)
          ++args_count;
        args[args_count - 1] *= 10;
        args[args_count - 1] += c - '0';
      } else if (c == ';') {
        ++args_count;
      } else {
        switch (c) {
        case 'H':
          cn_cursor_pos = 0;
          cn_update_cursor();
          break;
        case 'J':
          cn_clear();
          break;
        case 'K':
          if (args_count == 1)
            cn_erase_line();
          break;
        case 'f':
          if (args_count == 2)
            cn_goto(args[1], args[0]);
          break;
        default:
          break;
        }

        state = ESC_NORMAL;
      }
    } else {
      state = ESC_NORMAL;
    }
    break;
  }
  return 0;
#endif
}

ssize_t cn_read(struct inode *inode, dev_t dev, void *buf, size_t nbytes) {
  (void)dev;
  return kb_read(inode, buf, nbytes);
}

ssize_t cn_write(dev_t dev, const void *buf, size_t nbytes) {
  (void)dev;
  const char *p = (const char *)buf;
  for (size_t c = 0; c < nbytes; c++) {
    cn_putchar(dev, p[c]);
  }
  return nbytes;
}

void cn_clear(void) {
  for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
    cn_buffer[i] &= 0xFF00;
  }
}
