#include <errno.h>
#include <i386/idt.h>
#include <i386/kb.h>
#include <i386/pic.h>
#include <i386/uart.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/proc.h>
#include <sys/tty.h>

char kb_buffer[256];
volatile unsigned char kb_head, kb_tail;

const char *kb_map[127] = {
    "",     "\e",    "1", "2", "3", "4", "5", "6",    "7",    "8",     "9", "0",    "+",  "\\",   "\b", "\t",
    "q",    "w",     "e", "r", "t", "y", "u", "i",    "o",    "p",     "",  "",     "\n", "",     "a",  "s",
    "d",    "f",     "g", "h", "j", "k", "l", "",     "",     "|",     "",  "\"",   "z",  "x",    "c",  "v",
    "b",    "n",     "m", ",", ".", "-", "",  "",     "",     " ",     "",  "",     "",   "",     "",   "",
    "",     "",      "",  "",  "",  "",  "",  "\e[H", "\e[A", "\e[5~", "",  "\e[D", "",   "\e[C", "",   "\e[F",
    "\e[B", "\e[6~", "",  "",  "",  "",  "<", "",     "",     "",      "",  "",     "",   "",     "",   "",
    "",     "",      "",  "",  "",  "",  "",  "",     "",     "",      "",  "",     "",   "",     "",   "",
    "",     "",      "",  "",  "",  "",  "",  "",     "",     "",      "",  "",     "",   "",     ""};

const char *kb_map_shift[127] = {
    "",  "\e", "!", "'", "#", "$", "%",  "&", "/", "(", ")", "=", "?", "`", "\b", "\t", "Q", "W", "E", "R", "T", "Y",
    "U", "I",  "O", "P", "",  "",  "\n", "",  "A", "S", "D", "F", "G", "H", "J",  "K",  "L", "",  "",  "",  "",  "*",
    "Z", "X",  "C", "V", "B", "N", "M",  ";", ":", "_", "",  "",  "",  " ", "",   "",   "",  "",  "",  "",  "",  "",
    "",  "",   "",  "",  "",  "",  " ",  "",  "",  "",  "",  "",  "",  "",  "",   "",   "",  "",  "",  "",  ">", "",
    "",  "",   "",  "",  "",  "",  "",   "",  "",  "",  "",  "",  "",  "",  "",   "",   "",  "",  "",  "",  "",  "",
    "",  "",   "",  "",  "",  "",  "",   "",  "",  "",  "",  "",  "",  "",  "",   "",   ""};

const char *kb_map_ctrl[127] = {
    "",     "", "",    "", "", "", "", "", "", "", "",    "", "", "",    "", "", "",     "\x17", "", "", "", "",
    "\x15", "", "",    "", "", "", "", "", "", "", "\x4", "", "", "\x8", "", "", "\x0c", "",     "", "", "", "",
    "",     "", "\x3", "", "", "", "", "", "", "", "",    "", "", "",    "", "", "",     "",     "", "", "", "",
    "",     "", "",    "", "", "", "", "", "", "", "",    "", "", "",    "", "", "",     "",     "", "", "", "",
    "",     "", "",    "", "", "", "", "", "", "", "",    "", "", "",    "", "", "",     "",     "", "", "", "",
    "",     "", "",    "", "", "", "", "", "", "", "",    "", "", "",    "", "", ""};

typedef struct {
  uint8_t lshift;
  uint8_t rshift;
  uint8_t ctrl;
  uint8_t alt;
} kb_state_t;
kb_state_t kb_state;

void kb_irq();

void kb_input(char c) {
  if (c != 0) {
    // TODO: REMOVE kb_buffer?
    kb_buffer[kb_head++] = c;
    if (kb_head == kb_tail)
      kb_tail++;

    wakeup(kb_irq);

    tty_input(makedev(1, 0), c);
  }
}

void kb_cook(unsigned char raw) {
  const char *c = NULL;
  if ((raw & 0x7F) == 42) {
    kb_state.lshift = !kb_state.lshift;
  } else if ((raw & 0x7F) == 54) {
    kb_state.rshift = !kb_state.rshift;
  } else if ((raw & 0x7F) == 29) {
    kb_state.ctrl = !kb_state.ctrl;
  } else if ((raw & 0x7F) == 56) {
    kb_state.alt = !kb_state.alt;
  } else if (raw <= 0x7F) {
    if (kb_state.lshift == 1 || kb_state.rshift == 1) {
      c = kb_map_shift[raw];
    } else if (kb_state.ctrl == 1) {
      c = kb_map_ctrl[raw];
    } else {
      c = kb_map[raw];
    }
    // if (c == NULL || *c == 0) {
    //   uart_write(0, "raw=", 4);
    //   char h = ((raw >> 4) > 9) ? (raw >> 4) + 'A' - 10 : (raw >> 4) + '0';
    //   char l = ((raw & 15) > 9) ? (raw & 15) + 'A' - 10 : (raw & 15) + '0';
    //   uart_write(0, &h, 1);
    //   uart_write(0, &l, 1);
    //   uart_write(0, " ", 1);
    // }
  }

  if (c) {
    while (*c != 0) {
      kb_input(*c++);
    }
  }
}

void kb_irq() {
  if (inb(0x64) & 1) {
    unsigned char raw = inb(0x60);
    kb_cook(raw);
  }
  pic_ack();
}

void kb_init() {
  kb_head = 0;
  kb_tail = 0;

  memset(&kb_state, 0, sizeof(kb_state));

  idt_set_isr(1, kb_irq);
  pic_enable_interrupt(1);
}

ssize_t kb_read(struct inode *inode, char *buf, size_t nbytes) {
  size_t len = 0;

  while (len < nbytes) {
    while (kb_head == kb_tail && len == 0) {
      if (signal_pending(cur_proc))
        return -EINTR;

      iunlock(inode);
      sleep(kb_irq);
      ilock(inode);
    }
    if (kb_head == kb_tail)
      return len;
    buf[len++] = kb_buffer[kb_tail++];
  }
  return len;
}
