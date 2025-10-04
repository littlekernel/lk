LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_DEFINES += \
	ARM64_CPU_$(ARM_CPU)=1 \
	ARM_ISA_ARMV8=1 \
	IS_64BIT=1

MODULE_SRCS += \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/cache-ops.S \
	$(LOCAL_DIR)/exceptions.S \
	$(LOCAL_DIR)/exceptions_c.c \
	$(LOCAL_DIR)/fpu.c \
	$(LOCAL_DIR)/spinlock.S \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/thread.c \
    $(LOCAL_DIR)/mp.c \

# if its requested we build with SMP, default to 8 cpus
ifeq (true,$(call TOBOOL,$(WITH_SMP)))
SMP_MAX_CPUS ?= 8

GLOBAL_DEFINES += \
    WITH_SMP=1 \
    SMP_MAX_CPUS=$(SMP_MAX_CPUS)
else
GLOBAL_DEFINES += \
    SMP_MAX_CPUS=1
endif

ARCH_OPTFLAGS := -O2

# we have a mmu and want the vmm/pmm
WITH_KERNEL_VM ?= 1

ifeq (true,$(call TOBOOL,$(WITH_KERNEL_VM)))

MODULE_SRCS += \
	$(LOCAL_DIR)/mmu.c

ARM64_PAGE_SIZE ?= 4096

# platform/target/project is allowed to override the page size the kernel
# and user space will run at.
ifeq ($(ARM64_PAGE_SIZE), 4096)
# 48 bits of kernel and user address space (4 levels of page tables)
KERNEL_ASPACE_BASE ?= 0xffff000000000000
KERNEL_ASPACE_SIZE ?= 0x0001000000000000
USER_ASPACE_BASE   ?= 0x0000000001000000
USER_ASPACE_SIZE   ?= 0x0000fffffe000000
else ifeq ($(ARM64_PAGE_SIZE), 16384)
# 47 bits of kernel and user address space (3 levels of page tables)
GLOBAL_DEFINES += ARM64_LARGE_PAGESIZE_16K=1
KERNEL_ASPACE_BASE ?= 0xffff800000000000
KERNEL_ASPACE_SIZE ?= 0x0000800000000000
USER_ASPACE_BASE   ?= 0x0000000001000000
USER_ASPACE_SIZE   ?= 0x00007ffffe000000
else ifeq ($(ARM64_PAGE_SIZE), 65536)
# 42 bits of kernel and user address space (2 levels of page tables)
GLOBAL_DEFINES += ARM64_LARGE_PAGESIZE_64K=1
KERNEL_ASPACE_BASE ?= 0xfffffc0000000000
KERNEL_ASPACE_SIZE ?= 0x0000040000000000
USER_ASPACE_BASE   ?= 0x0000000001000000
USER_ASPACE_SIZE   ?= 0x000003fffe000000
else
$(error unsupported ARM64_PAGE_SIZE)
endif

GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=$(KERNEL_ASPACE_BASE) \
    KERNEL_ASPACE_SIZE=$(KERNEL_ASPACE_SIZE) \
    USER_ASPACE_BASE=$(USER_ASPACE_BASE) \
    USER_ASPACE_SIZE=$(USER_ASPACE_SIZE) \
    ARCH_HAS_MMU=1

KERNEL_BASE ?= $(KERNEL_ASPACE_BASE)
KERNEL_LOAD_OFFSET ?= 0

else # !WITH_KERNEL_VM

KERNEL_BASE ?= $(MEMBASE)
KERNEL_LOAD_OFFSET ?= 0

endif

GLOBAL_DEFINES += \
    KERNEL_BASE=$(KERNEL_BASE) \
    KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET)

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

# try to find the toolchain
include $(LOCAL_DIR)/toolchain.mk
TOOLCHAIN_PREFIX := $(ARCH_$(ARCH)_TOOLCHAIN_PREFIX)

ARCH_COMPILEFLAGS += $(ARCH_$(ARCH)_COMPILEFLAGS)
ARCH_COMPILEFLAGS += -ffixed-x18
ARCH_COMPILEFLAGS += -fno-omit-frame-pointer
ARCH_COMPILEFLAGS_NOFLOAT := -mgeneral-regs-only
ARCH_COMPILEFLAGS_FLOAT :=

ARCH_LDFLAGS += -z max-page-size=$(ARM64_PAGE_SIZE)

# Note: assumes the use of gcc and the user is not overriding CC which is set later in engine.mk
LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) -print-libgcc-file-name)

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

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/system-onesegment.ld

# rules for generating the linker script
$(BUILDDIR)/system-onesegment.ld: $(LOCAL_DIR)/system-onesegment.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

linkerscript.phony:
.PHONY: linkerscript.phony

MODULE_OPTIONS := extra_warnings

include make/module.mk
