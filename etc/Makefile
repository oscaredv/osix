include ../common.mk

CFLAGS+=-ffreestanding -fno-pie
CFLAGS+=-I. -I../include -I../sys

LDFLAGS=-nostdlib -T ../bin/link.ld

OBJDIR=../obj
LIBDIR=../obj/usr/lib
MAN1DIR=$(OBJDIR)/usr/man/man1
MAN5DIR=$(OBJDIR)/usr/man/man5

SRC=$(wildcard *.c)
BIN=$(addprefix $(OBJDIR)/etc/, $(SRC:%.c=%))
FILES=$(addprefix $(OBJDIR)/etc/, passwd group ttys motd)
MAN1=$(addprefix $(MAN1DIR)/, $(wildcard *.1))
MAN5=$(addprefix $(MAN5DIR)/, $(wildcard *.5))

.PHONY: all
all: $(BIN) $(FILES) $(MAN1) $(MAN5)

clean:
	$(RM) $(BIN) $(FILES)

$(OBJDIR)/etc/%: %.c Makefile $(LIBDIR)/libc.a
	@mkdir -p $(OBJDIR)/etc
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< $(LIBDIR)/libc.a

$(OBJDIR)/etc/%: %
	cp $< $@

$(OBJDIR)/usr/man/man1/%: %
	@mkdir -p $(MAN1DIR)
	cp $< $@

$(OBJDIR)/usr/man/man5/%: %
	@mkdir -p $(MAN5DIR)
	cp $< $@
