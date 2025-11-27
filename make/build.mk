# Rules for generating the final binary and any auxillary files generated as a result.

# use linker garbage collection, if requested
WITH_LINKER_GC ?= false
ifeq (true,$(call TOBOOL,$(WITH_LINKER_GC)))
GLOBAL_COMPILEFLAGS += -ffunction-sections -fdata-sections
GLOBAL_LDFLAGS += --gc-sections
GLOBAL_DEFINES += LINKER_GC=1
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
	$(NOECHO)$(LD) $(GLOBAL_LDFLAGS) $(ARCH_LDFLAGS) -d -T $(LINKER_SCRIPT) \
		$(addprefix -T,$(EXTRA_LINKER_SCRIPTS)) \
		$(ALLMODULE_OBJS) $(EXTRA_OBJS) $(LIBGCC) -Map=$(OUTELF).map -o $@

$(OUTELF).sym: $(OUTELF)
	$(info generating symbols: $@)
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) > $@
	$(NOECHO)echo "# vim: ts=8 nolist nowrap" >> $@

$(OUTELF).sym.sorted: $(OUTELF)
	$(info generating sorted symbols: $@)
	$(NOECHO)$(OBJDUMP) -t $< | $(CPPFILT) | sort > $@
	$(NOECHO)echo "# vim: ts=8 nolist nowrap" >> $@

$(OUTELF).lst: $(OUTELF)
	$(info generating listing: $@)
	$(NOECHO)$(OBJDUMP) $(ARCH_OBJDUMP_FLAGS) -d $< | $(CPPFILT) > $@
	$(NOECHO)echo "# vim: ts=8 nolist nowrap" >> $@

$(OUTELF).debug.lst: $(OUTELF)
	$(info generating listing: $@)
	$(NOECHO){ \
		$(OBJDUMP) $(ARCH_OBJDUMP_FLAGS) -l -S $< 2>$@.err | $(CPPFILT) > $@; \
		rc=$$?; \
		sed '/: failed to find source/d' $@.err >&2; \
		rm -f $@.err; \
		exit $$rc; \
	}
	$(NOECHO)echo "# vim: ts=8 nolist nowrap" >> $@

$(OUTELF).dump: $(OUTELF)
	$(info generating objdump: $@)
	$(NOECHO)$(OBJDUMP) -x $< | $(CPPFILT) > $@
	$(NOECHO)echo "# vim: ts=8 nolist nowrap" >> $@

$(OUTELF).size: $(OUTELF)
	$(info generating size map: $@)
	$(NOECHO)$(NM) -S --size-sort $< | $(CPPFILT) > $@
	$(NOECHO)echo "# vim: ts=8 nolist nowrap" >> $@

# generate a list of source files that potentially participate in this build.
# header file detection is a bit sloppy: it simply searches for every .h file inside
# the combined include paths. May pick up files that are not strictly speaking used.
# Alternate strategy that may work: union all of the .d files together and collect all
# of the used headers used there.
$(BUILDDIR)/srcfiles.txt: $(OUTELF) $(BUILDDIR)/include_paths.txt
	@$(MKDIR)
	$(info generating $@)
	$(NOECHO)echo $(sort $(ALLSRCS)) | tr ' ' '\n' > $@
	@for i in `cat $(BUILDDIR)/include_paths.txt`; do if [ -d $$i ]; then find $$i -type f -name \*.h; fi; done >> $@

# generate a list of all the include directories used in this project
$(BUILDDIR)/include_paths.txt: $(OUTELF)
	@$(MKDIR)
	$(info generating $@)
	$(NOECHO)echo $(subst -I,,$(sort $(GLOBAL_INCLUDES))) | tr ' ' '\n' > $@

#include arch/$(ARCH)/compile.mk

