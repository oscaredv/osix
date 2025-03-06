include ../common.mk

ASFLAGS+=-I. -I../include -I../sys

OBJDIR=../obj/usr/lib

.PHONY: all
all: $(OBJDIR)/crt0.o
	$(MAKE) -C libc all

.PHONY: clean
clean:
	$(RM) $(OBJDIR)/crt0.o
	$(MAKE) -C libc clean

$(OBJDIR)/%.o: %.S
	@mkdir -p $(OBJDIR)
	$(AS) -o $@ $(ASFLAGS) -c $<
