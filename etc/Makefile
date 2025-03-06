include ../common.mk

CFLAGS+=-ffreestanding -fno-pie
CFLAGS+=-I. -I../include -I../sys

LDFLAGS=-nostdlib -T ../bin/link.ld

OBJDIR=../obj
LIBDIR=../obj/usr/lib

SRC=$(wildcard *.c)
BIN=$(addprefix $(OBJDIR)/etc/, $(SRC:%.c=%))
FILES=$(addprefix $(OBJDIR)/etc/, passwd group ttys motd)

.PHONY: all
all: $(BIN) $(FILES)

clean:
	$(RM) $(BIN) $(FILES)

$(OBJDIR)/etc/%: %.c Makefile $(LIBDIR)/libc.a
	@mkdir -p $(OBJDIR)/etc
	$(CC) -o $@ $(CFLAGS) $(LDFLAGS) $< $(LIBDIR)/libc.a

$(OBJDIR)/etc/%: %
	cp $< $@

