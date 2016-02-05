LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x08000000
MEMBASE := 0x20000000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m0

ifeq ($(STM32_CHIP),stm32f072_x8)
GLOBAL_DEFINES += \
        STM32F072
MEMSIZE ?= 16384
endif
ifeq ($(STM32_CHIP),stm32f072_xB)
GLOBAL_DEFINES += \
        STM32F072
MEMSIZE ?= 16384
endif

GLOBAL_DEFINES += \
	USE_STDPERIPH_DRIVER \
	MEMSIZE=$(MEMSIZE)

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/timer.c \
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

include $(LOCAL_DIR)/STM32F0xx_StdPeriph_Driver/rules.mk $(LOCAL_DIR)/CMSIS/rules.mk

include make/module.mk
