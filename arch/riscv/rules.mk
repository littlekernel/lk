LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/start.S
MODULE_SRCS += $(LOCAL_DIR)/arch.c
MODULE_SRCS += $(LOCAL_DIR)/asm.S
MODULE_SRCS += $(LOCAL_DIR)/clint.c
MODULE_SRCS += $(LOCAL_DIR)/exceptions.c
MODULE_SRCS += $(LOCAL_DIR)/thread.c

GLOBAL_DEFINES += \
	SMP_MAX_CPUS=1

# set the default toolchain to riscv32 elf and set a #define
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := riscv32-elf-
endif

WITH_LINKER_GC ?= 0

cc-option = $(shell if test -z "`$(1) $(2) -S -o /dev/null -xc /dev/null 2>&1`"; \
	then echo "$(2)"; else echo "$(3)"; fi ;)

ARCH_COMPILEFLAGS :=
ARCH_OPTFLAGS := -O2

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) $(GLOBAL_CFLAGS) -print-libgcc-file-name)
$(info LIBGCC = $(LIBGCC))

KERNEL_BASE ?= $(MEMBASE)
KERNEL_LOAD_OFFSET ?= 0
ROMBASE ?= 0

GLOBAL_DEFINES += \
    ROMBASE=$(ROMBASE) \
    MEMBASE=$(MEMBASE) \
    MEMSIZE=$(MEMSIZE)

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/linker.ld

# rules for generating the linker
$(BUILDDIR)/linker.ld: $(LOCAL_DIR)/linker.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%ROMBASE%/$(ROMBASE)/;s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/;s/%VECTOR_BASE_PHYS%/$(VECTOR_BASE_PHYS)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

linkerscript.phony:
.PHONY: linkerscript.phony

LINKER_SCRIPT += $(BUILDDIR)/linker.ld

include make/module.mk
