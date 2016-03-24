LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x0
MEMBASE := 0x20000000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m3

ifeq ($(STM32_CHIP),stm32f107)
GLOBAL_DEFINES += \
	STM32F10X_CL=1	
MEMSIZE ?= 65536
endif
ifeq ($(STM32_CHIP),stm32f103_xl)
GLOBAL_DEFINES += \
	STM32F10X_XL=1
MEMSIZE ?= 65536
endif
ifeq ($(STM32_CHIP),stm32f103_hd)
GLOBAL_DEFINES += \
	STM32F10X_HD=1
MEMSIZE ?= 65536
endif
ifeq ($(STM32_CHIP),stm32f103_md)
GLOBAL_DEFINES += \
	STM32F10X_MD=1
MEMSIZE ?= 20480
endif
ifeq ($(STM32_CHIP),stm32f103_ld)
GLOBAL_DEFINES += \
	STM32F10X_LD=1
MEMSIZE ?= 20480
endif

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE)

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/flash_nor.c \

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
	platform/stm32f1xx/STM32F10x_StdPeriph_Driver \
	arch/arm/arm-m/systick \
	lib/cbuf

include make/module.mk
