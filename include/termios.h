#ifndef __INCLUDE_TERMIOS_H__
#define __INCLUDE_TERMIOS_H__

#define ECHO 8     // tty echo
#define ICANON 256 // enable terminal line dicipline

#define TCSANOW 0   // make change immediate
#define TCSAFLUSH 2 // drain output, flush input

struct termios {
  unsigned int c_lflag;
};

int tcgetattr(int fd, struct termios *t);
int tcsetattr(int fd, int opt_actions, struct termios *t);

#endif
