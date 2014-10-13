LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
	ARM64_CPU_$(ARM_CPU)=1 \
	ARM_ISA_ARMV8=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/exceptions.S \
	$(LOCAL_DIR)/exceptions_c.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/start.S \

#	$(LOCAL_DIR)/arm/start.S \
	$(LOCAL_DIR)/arm/cache-ops.S \
	$(LOCAL_DIR)/arm/cache.c \
	$(LOCAL_DIR)/arm/ops.S \
	$(LOCAL_DIR)/arm/faults.c \
	$(LOCAL_DIR)/arm/mmu.c \
	$(LOCAL_DIR)/arm/dcc.S

GLOBAL_DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=8192 \
	SMP_MAX_CPUS=1

ARCH_OPTFLAGS := -O2

# try to find the toolchain
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := aarch64-elf-
endif

FOUNDTOOL=$(shell which $(TOOLCHAIN_PREFIX)gcc)
ifeq ($(FOUNDTOOL),)
$(error cannot find toolchain, please set TOOLCHAIN_PREFIX or add it to your path)
endif
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

# make sure some bits were set up
MEMVARS_SET := 0
ifneq ($(MEMBASE),)
MEMVARS_SET := 1
endif
ifneq ($(MEMSIZE),)
MEMVARS_SET := 1
endif
ifeq ($(MEMVARS_SET),0)
$(error missing MEMBASE or MEMSIZE variable, please set in target rules.mk)
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) -print-libgcc-file-name)

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld

# rules for generating the linker script
$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld $(wildcard arch/*.ld)
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/" < $< > $@

include make/module.mk
