include ../common.mk

ASFLAGS+=-MMD -MP -I. -I../include
CFLAGS+=-MMD -MP -I. -I../include -ffreestanding -DKERNEL
LDFLAGS+=-nostdlib

OBJDIR=obj
KERNEL=../obj/osix

SRCS=$(wildcard kern/*.c sys/*.c dev/*.c i386/*.c i386/*.S libkern/*.c net/*.c netinet/*.c)
OBJS+=$(addprefix $(OBJDIR)/, $(patsubst %.S,%.o, $(patsubst %.c,%.o, $(SRCS))))
DEPS+=$(addprefix $(OBJDIR)/, $(patsubst %.S,%.d, $(patsubst %.c,%.d, $(SRCS))))

all: $(OBJDIR) $(KERNEL)

.PHONY: $(OBJDIR)
$(OBJDIR):
	@$(foreach path, $(sort $(dir $(OBJS))),mkdir -p $(path) ;)

$(KERNEL): $(OBJS) i386/link.ld
	bash conf/newvers.sh
	$(LD) $(LDFLAGS) -T i386/link.ld -o $(KERNEL) -ffreestanding $(OBJS) /tmp/vers.c -lgcc

.PHONY: iso
iso: $(OBJDIR)/iso

$(OBJDIR)/iso: $(KERNEL)
	@mkdir -p $(OBJDIR)/iso/boot/grub
	@cp ../obj/osix $(OBJDIR)/iso/boot
	@cp grub.cfg $(OBJDIR)/iso/boot/grub/
	@grub-mkrescue -o $(OBJDIR)/osix.iso $(OBJDIR)/iso

clean:
	$(RM) -r $(KERNEL) $(OBJDIR)

$(OBJDIR)/%.o: %.S
	$(AS) -o $@ $(ASFLAGS) -c $<

$(OBJDIR)/%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

-include $(DEPS)
