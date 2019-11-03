LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/start.S
MODULE_SRCS += $(LOCAL_DIR)/arch.c
MODULE_SRCS += $(LOCAL_DIR)/asm.S
MODULE_SRCS += $(LOCAL_DIR)/clint.c
MODULE_SRCS += $(LOCAL_DIR)/exceptions.c
MODULE_SRCS += $(LOCAL_DIR)/thread.c

GLOBAL_DEFINES += SMP_MAX_CPUS=1
GLOBAL_DEFINES += PLATFORM_HAS_DYNAMIC_TIMER=1

SUBARCH ?= 32

WITH_LINKER_GC ?= 0

KERNEL_BASE ?= $(MEMBASE)
KERNEL_LOAD_OFFSET ?= 0
ROMBASE ?= 0

GLOBAL_DEFINES += ROMBASE=$(ROMBASE)
GLOBAL_DEFINES += MEMBASE=$(MEMBASE)
GLOBAL_DEFINES += MEMSIZE=$(MEMSIZE)

# based on 32 or 64 bitness, select the right toolchain and some
# compiler codegen flags
ifeq ($(SUBARCH),32)

ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := riscv32-elf-
endif
ARCH_COMPILEFLAGS := -march=rv32imac -mabi=ilp32

else ifeq ($(SUBARCH),64)

ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := riscv64-elf-
endif
ARCH_COMPILEFLAGS := -march=rv64imafdc -mabi=lp64d -mcmodel=medany

else
$(error SUBARCH not set or set to something unknown)
endif

# embedded switch sets the default compile optimization and passes
# a flag to the code to switch other things on
ifeq (true,$(call TOBOOL,$(ARCH_RISCV_EMBEDDED)))
ARCH_OPTFLAGS ?= -Os
GLOBAL_DEFINES += ARCH_RISCV_EMBEDDED=1
else
ARCH_OPTFLAGS ?= -O2
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) $(GLOBAL_CFLAGS) -print-libgcc-file-name)
$(info LIBGCC = $(LIBGCC))

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/linker-onesegment.ld \
	$(BUILDDIR)/linker-twosegment.ld

# rules for generating the linker script
$(BUILDDIR)/linker-%.ld: $(LOCAL_DIR)/linker-%.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%BITS%/$(SUBARCH)/g;s/%ROMBASE%/$(ROMBASE)/;s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/;s/%VECTOR_BASE_PHYS%/$(VECTOR_BASE_PHYS)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

linkerscript.phony:
.PHONY: linkerscript.phony

# select the appropriate linker script based on if we're a one or two segment system
ifeq (true,$(call TOBOOL,$(ARCH_RISCV_TWOSEGMENT)))
GLOBAL_DEFINES += ARCH_RISCV_TWOSEGMENT=1
LINKER_SCRIPT += $(BUILDDIR)/linker-twosegment.ld
else
GLOBAL_DEFINES += ARCH_RISCV_TWOSEGMENT=0
LINKER_SCRIPT += $(BUILDDIR)/linker-onesegment.ld
endif

include make/module.mk
