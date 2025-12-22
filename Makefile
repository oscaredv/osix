OBJDIR=obj

.PHONY: all
all: world kernel

.PHONY: world
world:
	$(MAKE) -C lib
	$(MAKE) -C bin
	$(MAKE) -C etc

.PHONY: kernel
kernel:
	$(MAKE) -C sys

.PHONY: clean
clean:
	$(MAKE) -C sys clean
	$(MAKE) -C bin clean
	$(MAKE) -C lib clean
	$(RM) -r $(OBJDIR)

.PHONY: mount
mount:
	sudo modprobe nbd max_part=8
	sudo qemu-nbd --connect=/dev/nbd0 wd0.qcow2
	sudo mount /dev/nbd0p1 /mnt

.PHONY: umount
umount:
	sudo umount /mnt
	sudo qemu-nbd --disconnect /dev/nbd0

disconnect:
	sudo qemu-nbd --disconnect /dev/nbd0

QEMUFLAGS+=--no-reboot
#QEMUFLAGS+=-d int
QEMUFLAGS+=-serial mon:stdio
QEMUFLAGS+=-serial telnet:localhost:4321,server,nowait
QEMUFLAGS+=-hda wd0.qcow2
# User mode networking
#NETFLAGS+=-nic user,model=rtl8139,hostfwd=tcp::8080-:80,hostfwd=udp::8000-:8000
# Tap networking - run netstart.sh first
NETFLAGS+=-netdev tap,id=network0,ifname=tap0,script=no,downscript=no
NETFLAGS+=-device rtl8139,netdev=network0,mac=52:55:00:d1:55:01

.PHONY: run
run: all
	qemu-system-i386 -kernel obj/osix $(QEMUFLAGS)

.PHONY: network
network: all
	sudo qemu-system-i386 -kernel obj/osix $(QEMUFLAGS) $(NETFLAGS)

.PHONY: debug
debug: all
	qemu-system-i386 -s -S -kernel obj/osix $(QEMUFLAGS)

.PHONY: runserial
runserial: all
	qemu-system-i386 -nographic -kernel obj/osix $(QEMUFLAGS)

.PHONY: iso
iso: all
	$(MAKE) -C sys iso
	qemu-system-i386 -cdrom sys/obj/osix.iso -boot d $(QEMUFLAGS)

.PHONY: isoserial
isoserial: all
	$(MAKE) -C sys iso
	qemu-system-i386 -nographic -cdrom sys/obj/osix.iso -boot d $(QEMUFLAGS)

.PHONY: wd
wd: wd0.qcow2

%.qcow2:
	qemu-img create -f qcow2 $@ 64M


IMG_ALL=$(subst obj/,/mnt/,$(wildcard obj/* obj/*/* obj/*/*/* obj/*/*/*/*))
IMG_DIRS=$(abspath $(sort $(dir $(IMG_ALL))))
IMG_FILES=$(filter-out $(IMG_DIRS),$(IMG_ALL))
IMG_DEVS=$(addprefix /mnt/dev/, null zero console tty0 tty1 wd0 wd0p1)

.PHONY: install
install: all mount $(IMG_DEVS) $(IMG_FILES) /mnt/usr/oed /mnt/tmp umount

/mnt/usr/oed:
	sudo mkdir -p /mnt/usr/oed
	sudo chown 100:10 /mnt/usr/oed

/mnt/tmp:
	sudo mkdir /mnt/tmp
	sudo chmod 1777 /mnt/tmp

/mnt/%: obj/%
	@sudo mkdir -p $(IMG_DIRS)
	sudo cp $< $@

/mnt/dev:
	@sudo mkdir -p /mnt/dev

/mnt/dev/tty0: /mnt/dev
	sudo mknod -m=660 $@ c 3 0

/mnt/dev/tty1: /mnt/dev
	sudo mknod -m=660 $@ c 3 1

/mnt/dev/null: /mnt/dev
	sudo mknod -m=666 $@ c 0 0

/mnt/dev/zero: /mnt/dev
	sudo mknod -m=666 $@ c 2 0

/mnt/dev/console: /mnt/dev
	sudo mknod $@ c 1 0

/mnt/dev/wd0: /mnt/dev
	sudo mknod $@ b 0 0

/mnt/dev/wd0p1: /mnt/dev
	sudo mknod $@ b 0 1
