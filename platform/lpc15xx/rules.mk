LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script

ARCH := arm

ifeq ($(LPC_CHIP),LPC1549)
MEMSIZE ?= 36864
MEMBASE := 0x02000000
ROMBASE := 0x00000000
ARM_CPU := cortex-m3
endif

MODULE_DEFINES += PART_$(LPC_CHIP)

ifeq ($(MEMSIZE),)
$(error need to define MEMSIZE)
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/vectab.c \

#	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/usbc.c \


#	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/platform_early.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/init_clock.c \
	$(LOCAL_DIR)/init_clock_48mhz.c \
	$(LOCAL_DIR)/mux.c \
	$(LOCAL_DIR)/emac_dev.c

# use a two segment memory layout, where all of the read-only sections 
# of the binary reside in rom, and the read/write are in memory. The 
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set 
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	arch/arm/arm-m/systick \
	platform/lpc15xx/lpcopen \
	lib/cbuf

LPCSIGNEDBIN := $(OUTBIN).sign
LPCCHECK := $(LOCAL_DIR)/lpccheck.py
EXTRA_BUILDDEPS += $(LPCSIGNEDBIN)
GENERATED += $(LPCSIGNEDBIN)

$(LPCSIGNEDBIN): $(OUTBIN) $(LPCCHECK)
	@$(MKDIR)
	$(NOECHO)echo generating $@; \
	cp $< $@.tmp; \
	$(LPCCHECK) $@.tmp; \
	mv $@.tmp $@

include make/module.mk
