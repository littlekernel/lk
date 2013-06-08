LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x08000000
MEMBASE := 0x20000000
# default memsize, specific STM32_CHIP may override this
# and target/project may have already overridden
MEMSIZE ?= 131072

ARCH := arm
ARM_CPU := cortex-m3

ifeq ($(STM32_CHIP),stm32f207)
GLOBAL_DEFINES += \
	STM32F207=1	\
	STM32F2XX=1
FOUND_CHIP := true
endif
ifeq ($(STM32_CHIP),stm32f407)
GLOBAL_DEFINES += \
	STM32F407=1	\
	STM32F4XX=1
FOUND_CHIP := true
ARM_CPU := cortex-m4f
endif

ifeq ($(FOUND_CHIP),)
$(error unknown STM32F2xx chip $(STM32_CHIP))
endif

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE)

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/uart.c \

#	$(LOCAL_DIR)/flash_nor.c \
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
	lib/cbuf

include $(LOCAL_DIR)/STM32F2xx_StdPeriph_Driver/rules.mk $(LOCAL_DIR)/CMSIS/rules.mk

include make/module.mk
