include ../common.mk

ASFLAGS+=-I. -I../include

OBJDIR=../obj/usr/lib

.PHONY: all
all: ../include/sys ../include/$(MACHINE) $(OBJDIR)/crt0.o
	$(MAKE) -C libc all

../include/sys:
	cd ../include && ln -sf ../sys/sys sys

../include/$(MACHINE):
	cd ../include && ln -sf ../sys/$(MACHINE) machine

.PHONY: clean
clean:
	$(RM) $(OBJDIR)/crt0.o
	$(MAKE) -C libc clean

$(OBJDIR)/%.o: %.S
	@mkdir -p $(OBJDIR)
	$(AS) -o $@ $(ASFLAGS) -c $<
