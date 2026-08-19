LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x08000000
MEMBASE := 0x20000000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m33f

ifeq ($(STM32_CHIP),stm32h563_xi)
GLOBAL_DEFINES += STM32H563xx
# XXX workaround for uppercasing in GLOBAL_DEFINES
GLOBAL_COMPILEFLAGS += -DSTM32H563xx
# SRAM1, SRAM2 and SRAM3 are contiguous from MEMBASE: 256K + 64K + 320K
MEMSIZE ?= 0xa0000
else
$(error unknown STM32H5xx chip $(STM32_CHIP))
endif

LK_EMBEDDED := 1

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE)

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/rcc.c \
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
	platform/stm32 \
	platform/stm32h5xx/CMSIS \
	arch/arm/arm-m/systick \
	dev/gpio \
	lib/cbuf \
	lib/io

include make/module.mk
