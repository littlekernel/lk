LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/start.S \
	$(LOCAL_DIR)/asm.S \
	$(LOCAL_DIR)/arch.cpp \
	$(LOCAL_DIR)/exceptions.S \
	$(LOCAL_DIR)/exceptions_c.cpp \
	$(LOCAL_DIR)/thread.cpp

MODULE_DEPS += lib/libcpp

GLOBAL_DEFINES += \
	SMP_MAX_CPUS=1 \
	USE_BUILTIN_ATOMICS=0

# default toolchain
ifndef TOOLCHAIN_PREFIX
ifndef ARCH_sparc_TOOLCHAIN_PREFIX
TOOLCHAIN_PREFIX := sparc-elf-
else
TOOLCHAIN_PREFIX := $(ARCH_sparc_TOOLCHAIN_PREFIX)
endif
endif

ARCH_COMPILEFLAGS += -m32 -mcpu=v8 -mno-faster-structs
ARCH_OPTFLAGS ?= -O2

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(GLOBAL_COMPILEFLAGS) $(ARCH_COMPILEFLAGS) -print-libgcc-file-name)
$(info LIBGCC = $(LIBGCC))

KERNEL_BASE ?= $(MEMBASE)
KERNEL_LOAD_OFFSET ?= 0

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE) \
	KERNEL_BASE=$(KERNEL_BASE) \
	KERNEL_LOAD_OFFSET=$(KERNEL_LOAD_OFFSET)

GENERATED += \
	$(BUILDDIR)/linker.ld

$(BUILDDIR)/linker.ld: $(LOCAL_DIR)/linker.ld $(wildcard arch/*.ld) linkerscript.phony
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)sed "s/%MEMBASE%/$(MEMBASE)/;s/%MEMSIZE%/$(MEMSIZE)/;s/%KERNEL_BASE%/$(KERNEL_BASE)/;s/%KERNEL_LOAD_OFFSET%/$(KERNEL_LOAD_OFFSET)/" < $< > $@.tmp
	@$(call TESTANDREPLACEFILE,$@.tmp,$@)

linkerscript.phony:
.PHONY: linkerscript.phony

LINKER_SCRIPT += $(BUILDDIR)/linker.ld

include make/module.mk
