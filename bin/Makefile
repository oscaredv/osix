include ../common.mk

CFLAGS+=-ffreestanding -fno-pie
CFLAGS+=-I. -I../include

LDFLAGS=-nostdlib -T link.ld

OBJDIR=../obj
LIBDIR=../obj/usr/lib
MANDIR=$(OBJDIR)/usr/man/man1

SRC=$(wildcard *.c)
BIN=$(addprefix $(OBJDIR)/bin/, $(SRC:%.c=%) vi)
MAN=$(addprefix $(MANDIR)/, $(wildcard *.1))

.PHONY: all
all: ../include/sys ../include/$(MACHINE) $(BIN) $(MAN)

../include/sys:
	cd ../include && ln -sf ../sys/sys sys

../include/$(MACHINE):
	cd ../include && ln -sf ../sys/$(MACHINE) machine

clean:
	$(RM) $(BIN)

$(OBJDIR)/bin/%: %.c Makefile $(LIBDIR)/libc.a
	@mkdir -p $(OBJDIR)/bin
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< $(LIBDIR)/libc.a

$(OBJDIR)/bin/vi: vi/*.h vi/*.c Makefile $(LIBDIR)/libc.a
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) vi/*.c $(LIBDIR)/libc.a

$(OBJDIR)/usr/man/man1/%: %
	@mkdir -p $(MANDIR)
	cp $< $@

