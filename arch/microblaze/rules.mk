LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/exceptions.c \
	$(LOCAL_DIR)/thread.c \

#	$(LOCAL_DIR)/cache.c \
	$(LOCAL_DIR)/cache-ops.S \
	$(LOCAL_DIR)/ops.S \
	$(LOCAL_DIR)/mmu.c \
	$(LOCAL_DIR)/faults.c \
	$(LOCAL_DIR)/descriptor.c

GLOBAL_DEFINES += \
	SMP_MAX_CPUS=1

# set the default toolchain to microblaze elf and set a #define
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := microblaze-elf-
endif

WITH_LINKER_GC ?= 0

LITTLE_ENDIAN ?= 0

ifneq ($(LITTLE_ENDIAN),0)
ARCH_COMPILEFLAGS += -mlittle-endian
ARCH_LDFLAGS += -EL
GLOBAL_MODULE_LDFLAGS += -EL
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) $(GLOBAL_COMPILEFLAGS) -print-libgcc-file-name)
$(info LIBGCC = $(LIBGCC))

cc-option = $(shell if test -z "`$(1) $(2) -S -o /dev/null -xc /dev/null 2>&1`"; \
	then echo "$(2)"; else echo "$(3)"; fi ;)

ARCH_OPTFLAGS := -O2

ARCH_LDFLAGS += -relax

KERNEL_BASE ?= $(MEMBASE)
KERNEL_LOAD_OFFSET ?= 0
VECTOR_BASE_PHYS ?= 0

GLOBAL_DEFINES += \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE)

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/linker.ld

# rules for generating the linker
$(BUILDDIR)/linker.ld: $(LOCAL_DIR)/linker.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/;s/%VECTOR_BASE_PHYS%/$(VECTOR_BASE_PHYS)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

linkerscript.phony:
.PHONY: linkerscript.phony

LINKER_SCRIPT += $(BUILDDIR)/linker.ld

include make/module.mk
