LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

WITH_KERNEL_VM=1

ifeq ($(SUBARCH),x86-32)
MEMBASE ?= 0x00000000
KERNEL_BASE ?= 0x80000000
KERNEL_LOAD_OFFSET ?= 0x00200000
KERNEL_ASPACE_BASE ?= 0x80000000
KERNEL_ASPACE_SIZE ?= 0x7ff00000
USER_ASPACE_BASE   ?= 0
USER_ASPACE_SIZE   ?= 0x80000000

SUBARCH_DIR := $(LOCAL_DIR)/32
endif
ifeq ($(SUBARCH),x86-64)
GLOBAL_DEFINES += \
	IS_64BIT=1 \

MEMBASE ?= 0
KERNEL_BASE ?= 0xffffffff80000000
KERNEL_LOAD_OFFSET ?= 0x00200000
KERNEL_ASPACE_BASE ?= 0xffffff8000000000UL # -512GB
KERNEL_ASPACE_SIZE ?= 0x0000008000000000UL
USER_ASPACE_BASE   ?= 0x0000000000000000UL
USER_ASPACE_SIZE   ?= 0x0000800000000000UL
SUBARCH_DIR := $(LOCAL_DIR)/64
endif

SUBARCH_BUILDDIR := $(call TOBUILDDIR,$(SUBARCH_DIR))

GLOBAL_DEFINES += \
	ARCH_$(SUBARCH)=1 \
	MEMBASE=$(MEMBASE) \
	KERNEL_BASE=$(KERNEL_BASE) \
	KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET) \
	KERNEL_ASPACE_BASE=$(KERNEL_ASPACE_BASE) \
	KERNEL_ASPACE_SIZE=$(KERNEL_ASPACE_SIZE) \
	SMP_MAX_CPUS=1 \
	X86_WITH_FPU=1

MODULE_SRCS += \
	$(SUBARCH_DIR)/start.S \
	$(SUBARCH_DIR)/asm.S \
	$(SUBARCH_DIR)/exceptions.S \
	$(SUBARCH_DIR)/mmu.c \
	$(SUBARCH_DIR)/ops.S \
\
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/cache.c \
	$(LOCAL_DIR)/gdt.S \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/faults.c \
	$(LOCAL_DIR)/descriptor.c \
	$(LOCAL_DIR)/fpu.c

include $(LOCAL_DIR)/toolchain.mk

# set the default toolchain to x86 elf and set a #define
ifeq ($(SUBARCH),x86-32)
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := $(ARCH_x86_TOOLCHAIN_PREFIX)
endif
endif # SUBARCH x86-32
ifeq ($(SUBARCH),x86-64)
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := $(ARCH_x86_64_TOOLCHAIN_PREFIX)
endif
endif # SUBARCH x86-64

$(warning ARCH_x86_TOOLCHAIN_PREFIX = $(ARCH_x86_TOOLCHAIN_PREFIX))
$(warning ARCH_x86_64_TOOLCHAIN_PREFIX = $(ARCH_x86_64_TOOLCHAIN_PREFIX))
$(warning TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

cc-option = $(shell if test -z "`$(1) $(2) -S -o /dev/null -xc /dev/null 2>&1`"; \
	then echo "$(2)"; else echo "$(3)"; fi ;)

# disable SSP if the compiler supports it; it will break stuff
GLOBAL_CFLAGS += $(call cc-option,$(CC),-fno-stack-protector,)

ARCH_COMPILEFLAGS += -fasynchronous-unwind-tables
ARCH_COMPILEFLAGS += -gdwarf-2
ARCH_COMPILEFLAGS += -fno-pic
ARCH_LDFLAGS += -z max-page-size=4096

ifeq ($(SUBARCH),x86-64)
ARCH_COMPILEFLAGS += -fno-stack-protector
ARCH_COMPILEFLAGS += -mcmodel=kernel
ARCH_COMPILEFLAGS += -mno-red-zone
endif # SUBARCH x86-64

ARCH_OPTFLAGS := -O2

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) -print-libgcc-file-name)
$(warning LIBGCC = $(LIBGCC))

LINKER_SCRIPT += $(SUBARCH_BUILDDIR)/kernel.ld

# potentially generated files that should be cleaned out with clean make rule
GENERATED += $(SUBARCH_BUILDDIR)/kernel.ld

# rules for generating the linker scripts
$(SUBARCH_BUILDDIR)/kernel.ld: $(SUBARCH_DIR)/kernel.ld $(wildcard arch/*.ld)
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

include make/module.mk
