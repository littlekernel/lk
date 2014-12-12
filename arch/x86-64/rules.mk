LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

WITH_KERNEL_VM=1
GLOBAL_DEFINES += \
    KERNEL_ASPACE_BASE=0x00000000 \
    KERNEL_ASPACE_SIZE=0xc0000000
 
KERNEL_BASE ?= 0x00000000
KERNEL_LOAD_OFFSET ?= 0x200000

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
	$(LOCAL_DIR)/descriptor.c

# set the default toolchain to x86 elf and set a #define
ifndef TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := x86_64-elf-
endif

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -print-libgcc-file-name)
#$(info LIBGCC = $(LIBGCC))

cc-option = $(shell if test -z "`$(1) $(2) -S -o /dev/null -xc /dev/null 2>&1`"; \
	then echo "$(2)"; else echo "$(3)"; fi ;)

# disable SSP if the compiler supports it; it will break stuff
GLOBAL_CFLAGS += $(call cc-option,$(CC),-fno-stack-protector,)

GLOBAL_COMPILEFLAGS += -fasynchronous-unwind-tables
GLOBAL_COMPILEFLAGS += -gdwarf-2
GLOBAL_COMPILEFLAGS += -fno-stack-protector

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
