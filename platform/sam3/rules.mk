LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x80000
MEMBASE := 0x20000000

ARCH := arm
ARM_CPU := cortex-m3

ifeq ($(SAM_CHIP),sam3x8h)
GLOBAL_DEFINES += \
	__SAM3X8H__=1 \
	SAM3XA=1
MEMSIZE ?= 98304 
MEMBASE := 0x20070000
endif

ifeq ($(MEMSIZE),)
$(error need to define MEMSIZE)
endif

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/timer.c \

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
	lib/cbuf

include $(LOCAL_DIR)/cmsis/sam3x/rules.mk $(LOCAL_DIR)/drivers/rules.mk

include make/module.mk
