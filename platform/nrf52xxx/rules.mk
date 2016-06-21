LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

# ROMBASE, MEMBASE, and MEMSIZE are required for the linker script
ROMBASE := 0x0
MEMBASE := 0x20000000
# can be overridden by target

ARCH := arm
ARM_CPU := cortex-m4

ifeq ($(NRF52_CHIP),nrf52832-qfaa)
GLOBAL_DEFINES +=
MEMSIZE ?= 65536
endif
ifeq ($(NRF52_CHIP),nrf52832-qfab)
GLOBAL_DEFINES +=
MEMSIZE ?= 32768
endif
ifeq ($(NRF52_CHIP),nrf52832-chaa)
GLOBAL_DEFINES +=
MEMSIZE ?= 65536
endif
ifeq ($(NRF52_CHIP),nrf52832-chab)
GLOBAL_DEFINES +=
MEMSIZE ?= 32768
endif

GLOBAL_INCLUDES += $(LOCAL_DIR)

GLOBAL_DEFINES += \
	MEMSIZE=$(MEMSIZE)

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/vectab.c \
	$(LOCAL_DIR)/gpio.c \
    $(LOCAL_DIR)/timer.c \

LINKER_SCRIPT += \
	$(BUILDDIR)/system-twosegment.ld

MODULE_DEPS += \
	platform/nrf52 \
	lib/cbuf

include make/module.mk
