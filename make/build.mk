# use linker garbage collection, if requested
ifeq ($(WITH_LINKER_GC),1)
GLOBAL_COMPILEFLAGS += -ffunction-sections -fdata-sections
GLOBAL_LDFLAGS += --gc-sections
endif

ifneq (,$(EXTRA_BUILDRULES))
-include $(EXTRA_BUILDRULES)
endif

$(EXTRA_LINKER_SCRIPTS):

$(OUTBIN): $(OUTELF)
	$(info generating image: $@)
	$(NOECHO)$(SIZE) $<
	$(NOECHO)$(OBJCOPY) -O binary $< $@

$(OUTELF).hex: $(OUTELF)
	$(info generating hex file: $@)
	$(NOECHO)$(OBJCOPY) -O ihex $< $@

$(OUTELF): $(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LINKER_SCRIPT) $(EXTRA_LINKER_SCRIPTS)
	$(info linking $@)
	$(NOECHO)$(SIZE) -t --common $(sort $(ALLMODULE_OBJS)) $(EXTRA_OBJS)
	$(NOECHO)$(LD) $(GLOBAL_LDFLAGS) $(ARCH_LDFLAGS) -dT $(LINKER_SCRIPT) \
		$(addprefix -T,$(EXTRA_LINKER_SCRIPTS)) \
		$(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LIBGCC) -Map=$(OUTELF).map -o $@

$(OUTELF).sym: $(OUTELF)
	$(info generating symbols: $@)
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) > $@

$(OUTELF).sym.sorted: $(OUTELF)
	$(info generating sorted symbols: $@)
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) | sort > $@

$(OUTELF).lst: $(OUTELF)
	$(info generating listing: $@)
	$(NOECHO)$(OBJDUMP) $(ARCH_OBJDUMP_FLAGS) -d $< | $(CPPFILT) > $@

$(OUTELF).debug.lst: $(OUTELF)
	$(info generating listing: $@)
	$(NOECHO)$(OBJDUMP) $(ARCH_OBJDUMP_FLAGS) -S $< | $(CPPFILT) > $@

$(OUTELF).dump: $(OUTELF)
	$(info generating objdump: $@)
	$(NOECHO)$(OBJDUMP) -x $< > $@

$(OUTELF).size: $(OUTELF)
	$(info generating size map: $@)
	$(NOECHO)$(NM) -S --size-sort $< > $@

# print some information about the build
$(BUILDDIR)/srcfiles.txt: $(OUTELF)
	$(info generating $@)
	$(NOECHO)echo $(sort $(ALLSRCS)) | tr ' ' '\n' > $@

$(BUILDDIR)/include_paths.txt: $(OUTELF)
	$(info generating $@)
	$(NOECHO)echo $(subst -I,,$(sort $(GLOBAL_INCLUDES))) | tr ' ' '\n' > $@

#include arch/$(ARCH)/compile.mk

