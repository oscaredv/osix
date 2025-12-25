OBJDIR=../../../obj
MANDIR=$(OBJDIR)/usr/man/man2

MAN=$(addprefix $(MANDIR)/, $(wildcard *.2))

.PHONY: all
all: $(MAN)

.PHONY: clean
clean:
	$(RM) $(MAN)

$(OBJDIR)/usr/man/man2/%: %
	@mkdir -p $(MANDIR)
	cp $< $@

