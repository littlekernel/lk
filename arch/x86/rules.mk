LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

WITH_KERNEL_VM=1

GLOBAL_DEFINES += \
	MEMBASE=0x00200000U \
	KERNEL_ASPACE_BASE=0x00200000U \
	KERNEL_ASPACE_SIZE=0x7fe00000U \
	X86_WITH_FPU=1 \
	SMP_MAX_CPUS=1


KERNEL_BASE ?= 0x00200000
KERNEL_LOAD_OFFSET ?= 0x0

MODULE_SRCS += \
	$(LOCAL_DIR)/crt0.S \
	$(LOCAL_DIR)/arch.c \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/cache.c \
	$(LOCAL_DIR)/cache-ops.S \
	$(LOCAL_DIR)/ops.S \
	$(LOCAL_DIR)/thread.c \
	$(LOCAL_DIR)/mmu.c \
	$(LOCAL_DIR)/faults.c \
	$(LOCAL_DIR)/descriptor.c \
	$(LOCAL_DIR)/fpu.c

# set the default toolchain to x86 elf and set a #define
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := i386-elf-
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -print-libgcc-file-name)
#$(info LIBGCC = $(LIBGCC))

cc-option = $(shell if test -z "`$(1) $(2) -S -o /dev/null -xc /dev/null 2>&1`"; \
	then echo "$(2)"; else echo "$(3)"; fi ;)

# disable SSP if the compiler supports it; it will break stuff
GLOBAL_CFLAGS += $(call cc-option,$(CC),-fno-stack-protector,)

GLOBAL_COMPILEFLAGS += -fasynchronous-unwind-tables
GLOBAL_COMPILEFLAGS += -gdwarf-2

ARCH_OPTFLAGS := -O2

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/kernel.ld

# rules for generating the linker scripts
$(BUILDDIR)/kernel.ld: $(LOCAL_DIR)/kernel.ld $(wildcard arch/*.ld)
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)cp $< $@

include make/module.mk
