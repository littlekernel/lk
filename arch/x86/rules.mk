LOCAL_DIR := $(GET_LOCAL_DIR)

INCLUDES += \
	-I$(LOCAL_DIR)/include

BOOTOBJS += \
	$(LOCAL_DIR)/crt0.o

OBJS += \
	$(LOCAL_DIR)/arch.o \
	$(LOCAL_DIR)/asm.o \
	$(LOCAL_DIR)/cache.o \
	$(LOCAL_DIR)/cache-ops.o \
	$(LOCAL_DIR)/ops.o \
	$(LOCAL_DIR)/thread.o \
	$(LOCAL_DIR)/mmu.o \
	$(LOCAL_DIR)/faults.o \
	$(LOCAL_DIR)/descriptor.o

# set the default toolchain to x86 elf and set a #define
TOOLCHAIN_PREFIX ?= i386-elf-

LIBGCC := $(shell $(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -print-libgcc-file-name)
#$(info LIBGCC = $(LIBGCC))

cc-option = $(shell if test -z "`$(1) $(2) -S -o /dev/null -xc /dev/null 2>&1`"; \
	then echo "$(2)"; else echo "$(3)"; fi ;)

# disable SSP if the compiler supports it; it will break stuff
CFLAGS += $(call cc-option,$(CC),-fno-stack-protector,)

# potentially generated files that should be cleaned out with clean make rule
GENERATED += \
	$(BUILDDIR)/kernel.ld

# rules for generating the linker scripts

$(BUILDDIR)/kernel.ld: $(LOCAL_DIR)/kernel.ld
	@echo generating $@
	@$(MKDIR)
	$(NOECHO)cp $< $@
