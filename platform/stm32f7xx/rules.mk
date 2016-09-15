LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE ?= 0x00200000
MEMBASE ?= 0x20010000
# default memsize, specific STM32_CHIP may override this
# and target/project may have already overridden
MEMSIZE ?= 0x40000

ARCH := arm
ARM_CPU := cortex-m7-fpu-sp-d16

ifeq ($(STM32_CHIP),stm32f746)
GLOBAL_DEFINES += STM32F746xx
# XXX workaround for uppercasing in GLOBAL_DEFINES
GLOBAL_COMPILEFLAGS += -DSTM32F746xx
FOUND_CHIP := true
endif

ifeq ($(STM32_CHIP),stm32f756)
GLOBAL_DEFINES += STM32F746xx
# XXX workaround for uppercasing in GLOBAL_DEFINES
GLOBAL_COMPILEFLAGS += -DSTM32F746xx
FOUND_CHIP := true
endif

ifeq ($(FOUND_CHIP),)
$(error unknown STM32F7xx chip $(STM32_CHIP))
endif

LK_HEAP_IMPLEMENTATION ?= miniheap

GLOBAL_DEFINES += \
	PLATFORM_SUPPORTS_PANIC_SHELL=1 \
    NOVM_MAX_ARENAS=2 \
    CONSOLE_HAS_INPUT_BUFFER=1

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/eth.c \
	$(LOCAL_DIR)/flash.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/usbc.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/sdram.c \
	$(LOCAL_DIR)/qspi.c

# use a two segment memory layout, where all of the read-only sections 
# of the binary reside in rom, and the read/write are in memory. The 
# ROMBASE, MEMBASE, and MEMSIZE make variables are required to be set 
# for the linker script to be generated properly.
#
LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	platform/stm32 \
	platform/stm32f7xx/STM32F7xx_HAL_Driver \
	arch/arm/arm-m/systick \
	dev/gpio \
	dev/usb \
	lib/bio \
	lib/cbuf

include make/module.mk
