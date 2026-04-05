include ../common.mk

ASFLAGS+=-I. -I../include

OBJDIR=../obj/usr/lib

.PHONY: all
all: ../include/sys ../include/machine $(OBJDIR)/crt0.o
	$(MAKE) -C libc all

../include/sys:
	cd ../include && ln -snf ../sys/sys sys

../include/machine:
	cd ../include && ln -snf ../sys/$(MACHINE) machine

.PHONY: clean
clean:
	$(RM) $(OBJDIR)/crt0.o
	$(RM) ../include/sys ../include/machine
	$(MAKE) -C libc clean

$(OBJDIR)/%.o: %.S
	@mkdir -p $(OBJDIR)
	$(AS) -o $@ $(ASFLAGS) -c $<
