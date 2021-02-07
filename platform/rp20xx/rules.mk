LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x10000000
MEMBASE := 0x20000000
MEMSIZE := 0x00042000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m0plus

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE)

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/vectab.c

# use a two segment memory layout, where all of the read-only sections
# of the binary reside in rom, and the read/write are in memory. The
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	arch/arm/arm-m/systick \
	lib/cbuf

# take the result of the build and generate a uf2 file
UF2BIN := $(basename $(OUTBIN)).uf2
UF2CONV_TOOL := $(LOCAL_DIR)/tools/uf2conv.py
FAMILY_ID := 0xe48bff56 # UF2 family id
$(UF2BIN): $(OUTBIN) $(UF2CONV_TOOL)
	@$(MKDIR)
	$(NOECHO)echo generating $@; \
	$(UF2CONV_TOOL) -b $(ROMBASE) -f $(FAMILY_ID) -c -o $@ $<

EXTRA_BUILDDEPS += $(UF2BIN)
GENERATED += $(UF2BIN)

include make/module.mk
