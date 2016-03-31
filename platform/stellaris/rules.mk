LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script

ARCH := arm

ifeq ($(STELLARIS_CHIP),LM4F120H5QR)
MEMSIZE ?= 32768
MEMBASE := 0x20000000
ROMBASE := 0x00000000
ARM_CPU := cortex-m4f
GLOBAL_DEFINES += \
    TARGET_IS_BLIZZARD_RA1 \
    __FPU_PRESENT=1
endif
ifeq ($(STELLARIS_CHIP),LM3S6965)
MEMSIZE ?= 65536
MEMBASE := 0x20000000
ROMBASE := 0x00000000
ARM_CPU := cortex-m3
GLOBAL_DEFINES += TARGET_IS_FURY_RA2
endif

GLOBAL_DEFINES += PART_$(STELLARIS_CHIP)

ifeq ($(MEMSIZE),)
$(error need to define MEMSIZE)
endif

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/usbc.c \
	$(LOCAL_DIR)/vectab.c \

# use a two segment memory layout, where all of the read-only sections 
# of the binary reside in rom, and the read/write are in memory. The 
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set 
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	platform/stellaris/ti-driverlib \
	arch/arm/arm-m/systick \
	lib/cbuf \
	dev/usb

include make/module.mk
