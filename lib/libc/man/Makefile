OBJDIR=../../../obj
MAN2DIR=$(OBJDIR)/usr/man/man2
MAN3DIR=$(OBJDIR)/usr/man/man3

MAN2=$(addprefix $(MAN2DIR)/, $(wildcard *.2))
MAN3=$(addprefix $(MAN3DIR)/, $(wildcard *.3))
LINKS=$(MAN3DIR)/free.3 $(MAN2DIR)/sbrk.2

.PHONY: all
all: $(MAN2) $(MAN3) $(LINKS)

.PHONY: clean
clean:
	$(RM) $(MAN2) $(MAN3) $(LINKS)

$(MAN2DIR)/%: %
	@mkdir -p $(MAN2DIR)
	cp $< $@

$(MAN3DIR)/%: %
	@mkdir -p $(MAN3DIR)
	cp $< $@

$(MAN3DIR)/free.3: malloc.3
	@mkdir -p $(MAN3DIR)
	cd $(MAN3DIR) && ln -sf malloc.3 free.3

$(MAN2DIR)/sbrk.2: brk.2
	@mkdir -p $(MAN2DIR)
	cd $(MAN2DIR) && ln -sf brk.2 sbrk.2
