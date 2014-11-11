LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
	ARM64_CPU_$(ARM_CPU)=1 \
	ARM_ISA_ARMV8=1 \
	IS_64BIT=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/exceptions.S \
	$(LOCAL_DIR)/exceptions_c.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/spinlock.S \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/cache-ops.S \

#	$(LOCAL_DIR)/arm/start.S \
	$(LOCAL_DIR)/arm/cache.c \
	$(LOCAL_DIR)/arm/ops.S \
	$(LOCAL_DIR)/arm/faults.c \
	$(LOCAL_DIR)/arm/dcc.S

GLOBAL_DEFINES += \
	ARCH_DEFAULT_STACK_SIZE=8192 \
	SMP_MAX_CPUS=1

ARCH_OPTFLAGS := -O2

# we have a mmu and want the vmm/pmm
WITH_KERNEL_VM ?= 1

ifeq ($(WITH_KERNEL_VM),1)

MODULE_SRCS += \
	$(LOCAL_DIR)/mmu.c

KERNEL_ASPACE_BASE ?= 0xffff000000000000
KERNEL_ASPACE_SIZE ?= 0x0001000000000000

GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=$(KERNEL_ASPACE_BASE) \
    KERNEL_ASPACE_SIZE=$(KERNEL_ASPACE_SIZE)

KERNEL_BASE ?= 0xffff000000000000
KERNEL_LOAD_OFFSET ?= 0

GLOBAL_DEFINES += \
    KERNEL_BASE=$(KERNEL_BASE) \
    KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET)

else

KERNEL_BASE ?= $(MEMBASE)
KERNEL_LOAD_OFFSET ?= 0

endif


# try to find the toolchain
include $(LOCAL_DIR)/toolchain.mk
TOOLCHAIN_PREFIX := $(ARCH_$(ARCH)_TOOLCHAIN_PREFIX)
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

ARCH_COMPILEFLAGS += $(ARCH_$(ARCH)_COMPILEFLAGS)

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
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@

include make/module.mk
