include ../../common.mk

CFLAGS+=-ffreestanding -fno-pie
CFLAGS+=-I. -I../../include -I../../sys
LDFLAGS+=-nostdlib

OBJDIR=../../obj/usr/lib/libc
LIBDIR=../../obj/usr/lib

SRCS=$(wildcard *.c)
OBJS=$(addprefix $(OBJDIR)/, $(SRCS:%.c=%.o))

.PHONY: all
all: $(LIBDIR)/libc.a

.PHONY: clean
clean:
	$(RM) $(LIBDIR)/libc.a $(OBJS)

$(LIBDIR)/libc.a: $(OBJS) ../crt0.S
	@mkdir -p $(LIBDIR)
	ar rcs $(LIBDIR)/libc.a $(OBJS)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -c -o $@ $<

