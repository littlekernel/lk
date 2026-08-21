# Rules that build the kernel symbol table lib/symtab searches at runtime.
#
# The table maps text addresses to names, so it can only be generated once the
# linker has decided where everything goes, but it also has to be part of the
# image it describes. That takes three link passes:
#
#   1. link against an empty table (symtab_placeholder.c) to find out where
#      the functions land
#   2. link against a table built from those symbols. Adding a table of real
#      size moves code: on RISC-V it changes how far the table is from the
#      global pointer, which changes whether accesses to it relax to the
#      short gp relative form, which resizes the code that reads the table.
#   3. link against a table built from the symbols of pass 2. This one is
#      correct, because a table's size depends only on the set of symbol
#      names and not on their addresses: the pass 2 and pass 3 tables are the
#      same size, so the two images have identical layout, so the addresses
#      the pass 3 table holds are the addresses the final image ended up with.
#
# The verify step checks that last claim on every build by diffing the symbols
# of pass 2 against the final image, rather than trusting it.
#
# build.mk includes this file via EXTRA_BUILDRULES, which happens after every
# module has been processed and before the final link rule is defined, so the
# object lists here are complete.

SYMTAB_PASS1_ELF := $(BUILDDIR)/lk-symtab-pass1.elf
SYMTAB_PASS1_NM := $(BUILDDIR)/lk-symtab-pass1.nm
SYMTAB_PASS2_ELF := $(BUILDDIR)/lk-symtab-pass2.elf
SYMTAB_PASS2_NM := $(BUILDDIR)/lk-symtab-pass2.nm
SYMTAB_FINAL_NM := $(BUILDDIR)/lk-symtab-final.nm
SYMTAB_PLACEHOLDER_OBJ := $(BUILDDIR)/symtab_placeholder.o
SYMTAB_PASS1_DATA_C := $(BUILDDIR)/symtab_data_pass1.c
SYMTAB_PASS1_DATA_OBJ := $(BUILDDIR)/symtab_data_pass1.o
SYMTAB_DATA_C := $(BUILDDIR)/symtab_data.c
SYMTAB_DATA_OBJ := $(BUILDDIR)/symtab_data.o
SYMTAB_VERIFY_STAMP := $(BUILDDIR)/symtab-verify.stamp
SYMTAB_GENERATOR := scripts/gen-symtab.py

# Everything the final image is linked from except the table itself.
SYMTAB_LINK_OBJS := $(ALLMODULE_OBJS) $(filter-out $(SYMTAB_DATA_OBJ),$(EXTRA_OBJS))

# Compile one of the table source files the way compile.mk would, minus the LTO
# flags: the table is data with no calls into or out of it, so there is nothing
# for LTO to do, and a plain object keeps the passes comparable.
define SYMTAB_COMPILE
	@$(MKDIR)
	$(info compiling $<)
	$(NOECHO)$(CC) $(GLOBAL_OPTFLAGS) $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) \
		$(GLOBAL_CFLAGS) $(ARCH_CFLAGS) $(GLOBAL_INCLUDES) -c $< -o $@
endef

# An intermediate link, identical to the real one in make/build.mk except for
# the table object it picks up and the lack of a map file.
# $1 - the table object to link in
define SYMTAB_LINK
	$(info linking $@)
	$(NOECHO)$(LD) $(GLOBAL_LDFLAGS) $(ARCH_LDFLAGS) -d -T $(LINKER_SCRIPT) \
		$(addprefix -T,$(EXTRA_LINKER_SCRIPTS)) \
		$(SYMTAB_LINK_OBJS) $1 $(LIBGCC) -o $@
endef

# TESTANDREPLACEFILE keeps an unchanged table from touching off another link.
define SYMTAB_GENERATE
	$(info generating $@)
	$(NOECHO)$(SYMTAB_GENERATOR) -o $@.tmp < $<
	$(NOECHO)$(call TESTANDREPLACEFILE,$@.tmp,$@)
endef

$(SYMTAB_PLACEHOLDER_OBJ): $(SYMTAB_LOCAL_DIR)/symtab_placeholder.c $(GLOBAL_SRCDEPS)
	$(SYMTAB_COMPILE)

$(SYMTAB_PASS1_DATA_OBJ): $(SYMTAB_PASS1_DATA_C) $(GLOBAL_SRCDEPS)
	$(SYMTAB_COMPILE)

$(SYMTAB_DATA_OBJ): $(SYMTAB_DATA_C) $(GLOBAL_SRCDEPS)
	$(SYMTAB_COMPILE)

$(SYMTAB_PASS1_ELF): $(SYMTAB_LINK_OBJS) $(SYMTAB_PLACEHOLDER_OBJ) $(LINKER_SCRIPT) $(EXTRA_LINKER_SCRIPTS)
	$(call SYMTAB_LINK,$(SYMTAB_PLACEHOLDER_OBJ))

$(SYMTAB_PASS2_ELF): $(SYMTAB_LINK_OBJS) $(SYMTAB_PASS1_DATA_OBJ) $(LINKER_SCRIPT) $(EXTRA_LINKER_SCRIPTS)
	$(call SYMTAB_LINK,$(SYMTAB_PASS1_DATA_OBJ))

$(SYMTAB_PASS1_NM): $(SYMTAB_PASS1_ELF)
	$(NOECHO)$(NM) -n --defined-only $< > $@

$(SYMTAB_PASS2_NM): $(SYMTAB_PASS2_ELF)
	$(NOECHO)$(NM) -n --defined-only $< > $@

$(SYMTAB_PASS1_DATA_C): $(SYMTAB_PASS1_NM) $(SYMTAB_GENERATOR)
	$(SYMTAB_GENERATE)

$(SYMTAB_DATA_C): $(SYMTAB_PASS2_NM) $(SYMTAB_GENERATOR)
	$(SYMTAB_GENERATE)

# Fail the build if the final link moved any of the text the table names,
# which would leave every symbol in the image silently off by some amount.
$(SYMTAB_VERIFY_STAMP): $(OUTELF) $(SYMTAB_PASS2_NM) $(SYMTAB_GENERATOR)
	$(info verifying symbol table: $@)
	$(NOECHO)$(NM) -n --defined-only $(OUTELF) > $(SYMTAB_FINAL_NM)
	$(NOECHO)$(SYMTAB_GENERATOR) --verify $(SYMTAB_PASS2_NM) $(SYMTAB_FINAL_NM)
	$(NOECHO)touch $@

EXTRA_BUILDDEPS += $(SYMTAB_VERIFY_STAMP)

GENERATED += \
	$(SYMTAB_PASS1_ELF) \
	$(SYMTAB_PASS1_NM) \
	$(SYMTAB_PASS2_ELF) \
	$(SYMTAB_PASS2_NM) \
	$(SYMTAB_FINAL_NM) \
	$(SYMTAB_PLACEHOLDER_OBJ) \
	$(SYMTAB_PASS1_DATA_C) \
	$(SYMTAB_PASS1_DATA_OBJ) \
	$(SYMTAB_DATA_C) \
	$(SYMTAB_DATA_OBJ) \
	$(SYMTAB_VERIFY_STAMP)
