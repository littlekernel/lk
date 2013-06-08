$(OUTBIN): $(OUTELF)
	@echo generating image: $@
	$(NOECHO)$(SIZE) $<
	$(NOECHO)$(OBJCOPY) -O binary $< $@

$(OUTELF).hex: $(OUTELF)
	@echo generating hex file: $@
	$(NOECHO)$(OBJCOPY) -O ihex $< $@

$(OUTELF): $(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LINKER_SCRIPT)
	@echo linking $@
	$(NOECHO)$(SIZE) -t --common $(sort $(ALLMODULE_OBJS))
	$(NOECHO)$(LD) $(GLOBAL_LDFLAGS) -T $(LINKER_SCRIPT) $(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LIBGCC) -o $@

$(OUTELF).sym: $(OUTELF)
	@echo generating symbols: $@
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) > $@

$(OUTELF).lst: $(OUTELF)
	@echo generating listing: $@
	$(NOECHO)$(OBJDUMP) -Mreg-names-raw -d $< | $(CPPFILT) > $@

$(OUTELF).debug.lst: $(OUTELF)
	@echo generating listing: $@
	$(NOECHO)$(OBJDUMP) -Mreg-names-raw -S $< | $(CPPFILT) > $@

$(OUTELF).size: $(OUTELF)
	@echo generating size map: $@
	$(NOECHO)$(NM) -S --size-sort $< > $@

#include arch/$(ARCH)/compile.mk

