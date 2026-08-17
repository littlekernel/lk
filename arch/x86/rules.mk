LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_OPTIONS := extra_warnings
MODULE_DEPS := lib/fixed_point

# x86 code always runs with the mmu enabled
WITH_KERNEL_VM := 1
ifneq ($(CPU),legacy)
WITH_SMP ?= 1
else
WITH_SMP ?= 0
endif

ifeq ($(SUBARCH),x86-32)
MEMBASE ?= 0x00000000
KERNEL_BASE ?= 0x80000000
KERNEL_LOAD_OFFSET ?= 0x00200000
KERNEL_ASPACE_BASE ?= 0x80000000
KERNEL_ASPACE_SIZE ?= 0x7ff00000
USER_ASPACE_BASE   ?= 0x1000     # 4KB
USER_ASPACE_SIZE   ?= 0x7fffe000 # 2GB - 2*4KB

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
USER_ASPACE_BASE   ?= 0x0000000000001000UL # 4KB
USER_ASPACE_SIZE   ?= 0x00007fffffffe000UL # ((1<<47) - 2*4KB)
SUBARCH_DIR := $(LOCAL_DIR)/64
endif

SUBARCH_BUILDDIR := $(call TOBUILDDIR,$(SUBARCH_DIR))

# x86 has no static memory size (RAM is discovered at runtime via multiboot/e820).
# This is a nominal placeholder solely so MEMSIZE is universally defined for
# preprocessor use (see app/tests/benchmarks.c, lib/heap/test/heap_tests.c) - it is
# NOT used by the linker (the x86 linker scripts have no %MEMSIZE% token) and must
# not be read as authoritative. Keep it well above 1MB or x86 test builds silently
# flip to the small-buffer branch in benchmarks.c.
MEMSIZE ?= 0x08000000 # 128MB nominal placeholder, not authoritative

GLOBAL_DEFINES += \
	ARCH_$(SUBARCH)=1 \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE) \
	KERNEL_BASE=$(KERNEL_BASE) \
	KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET) \
	KERNEL_ASPACE_BASE=$(KERNEL_ASPACE_BASE) \
	KERNEL_ASPACE_SIZE=$(KERNEL_ASPACE_SIZE) \
	USER_ASPACE_BASE=$(USER_ASPACE_BASE) \
	USER_ASPACE_SIZE=$(USER_ASPACE_SIZE) \
	ARCH_HAS_MMU=1

ifeq ($(WITH_SMP),1)
SMP_MAX_CPUS ?= 16
GLOBAL_DEFINES += \
    WITH_SMP=1 \
    SMP_MAX_CPUS=$(SMP_MAX_CPUS)
else
GLOBAL_DEFINES += \
    SMP_MAX_CPUS=1
endif

MODULE_SRCS += \
	$(SUBARCH_DIR)/start.S \
\
	$(SUBARCH_DIR)/asm.S \
	$(SUBARCH_DIR)/exceptions.S \
	$(SUBARCH_DIR)/gdt.S \
	$(SUBARCH_DIR)/mmu.c \
	$(SUBARCH_DIR)/ops.S \
	$(SUBARCH_DIR)/spinlock.S \
\
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/cache.c \
	$(LOCAL_DIR)/clocks.c \
	$(LOCAL_DIR)/descriptor.c \
	$(LOCAL_DIR)/faults.c \
	$(LOCAL_DIR)/feature.c \
	$(LOCAL_DIR)/ioapic.c \
	$(LOCAL_DIR)/lapic.c \
	$(LOCAL_DIR)/mtrr.c \
	$(LOCAL_DIR)/mp.c \
	$(LOCAL_DIR)/pv.c \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/uarch.c \

ifeq ($(SUBARCH),x86-64)
# the PE/COFF header that makes lk.bin double as a UEFI application, plus the
# EFI boot stub. The header is inert data on multiboot boots, so it is always
# built.
MODULE_SRCS += \
	$(SUBARCH_DIR)/efi-header.S
MODULE_DEPS += \
	$(SUBARCH_DIR)/efi
endif

# legacy x86's dont have fpu support
ifneq ($(CPU),legacy)
GLOBAL_DEFINES += \
	X86_WITH_FPU=1

MODULE_SRCS += \
	$(LOCAL_DIR)/fpu.c
else
GLOBAL_DEFINES += WITH_NO_FP=1
endif

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

$(info ARCH_x86_TOOLCHAIN_PREFIX = $(ARCH_x86_TOOLCHAIN_PREFIX))
$(info ARCH_x86_64_TOOLCHAIN_PREFIX = $(ARCH_x86_64_TOOLCHAIN_PREFIX))
$(info TOOLCHAIN_PREFIX = $(TOOLCHAIN_PREFIX))

cc-option = $(shell if test -z "`$(1) $(2) -S -o /dev/null -xc /dev/null 2>&1`"; \
	then echo "$(2)"; else echo "$(3)"; fi ;)

# disable SSP if the compiler supports it; it will break stuff
GLOBAL_CFLAGS += $(call cc-option,$(CC),-fno-stack-protector,)

ARCH_COMPILEFLAGS += -fasynchronous-unwind-tables
ARCH_COMPILEFLAGS += -fno-pic
ARCH_LDFLAGS += -z max-page-size=4096

ifeq ($(SUBARCH),x86-64)
ARCH_COMPILEFLAGS += -fno-stack-protector
ARCH_COMPILEFLAGS += -mcmodel=kernel
ARCH_COMPILEFLAGS += -mno-red-zone
endif # SUBARCH x86-64

# set switches to generate/not generate fpu code
ARCH_COMPILEFLAGS_FLOAT +=
ARCH_COMPILEFLAGS_NOFLOAT += -mgeneral-regs-only

# select default optimizations for different target cpu levels
ifeq ($(CPU),legacy)
# compile for 386 when selecting 'legacy' cpu support
ARCH_COMPILEFLAGS += -march=i386
ARCH_OPTFLAGS ?= -Os
GLOBAL_DEFINES += X86_LEGACY=1
else ifeq ($(SUBARCH),x86-32)
ARCH_COMPILEFLAGS += -march=i686
ARCH_OPTFLAGS ?= -O2
GLOBAL_DEFINES += X86_LEGACY=0
else ifeq ($(SUBARCH),x86-64)
ARCH_COMPILEFLAGS += -march=x86-64
ARCH_OPTFLAGS ?= -O2
GLOBAL_DEFINES += X86_LEGACY=0
endif

ifeq ($(SUBARCH),x86-64)
CLANG_ARCH_TRIPLE ?= x86_64-elf
else
CLANG_ARCH_TRIPLE ?= i386-elf
endif

LINKER_SCRIPT += $(SUBARCH_BUILDDIR)/kernel.ld

ifeq ($(SUBARCH),x86-64)
# lk.bin already carries a PE/COFF header (arch/x86/64/efi-header.S), so the
# UEFI application is a plain copy with the conventional extension
OUTEFI := $(basename $(OUTBIN)).efi
$(OUTEFI): $(OUTBIN)
	@echo generating $@
	$(NOECHO)cp $< $@
EXTRA_BUILDDEPS += $(OUTEFI)
GENERATED += $(OUTEFI)
endif

# potentially generated files that should be cleaned out with clean make rule
GENERATED += $(SUBARCH_BUILDDIR)/kernel.ld

# rules for generating the linker scripts
$(SUBARCH_BUILDDIR)/kernel.ld: $(SUBARCH_DIR)/kernel.ld $(wildcard arch/*.ld)
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

include make/module.mk
