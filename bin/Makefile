include ../common.mk

CFLAGS+=-ffreestanding -fno-pie
CFLAGS+=-I. -I../include

LDFLAGS=-nostdlib -T link.ld

OBJDIR=../obj
LIBDIR=../obj/usr/lib
MANDIR=$(OBJDIR)/usr/man/man1

SRC=$(wildcard *.c)
BIN=$(addprefix $(OBJDIR)/bin/, $(SRC:%.c=%))
MAN=$(addprefix $(MANDIR)/, $(wildcard *.1))

.PHONY: all
all: ../include/sys ../include/machine $(BIN) $(MAN)

../include/sys:
	cd ../include && ln -snf ../sys/sys sys

../include/machine:
	cd ../include && ln -snf ../sys/$(MACHINE) machine

clean:
	$(RM) $(BIN)
	$(RM) ../include/sys ../include/machine

$(OBJDIR)/bin/%: %.c Makefile $(LIBDIR)/libc.a
	@mkdir -p $(OBJDIR)/bin
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< $(LIBDIR)/libc.a

$(OBJDIR)/usr/man/man1/%: %
	@mkdir -p $(MANDIR)
	cp $< $@

