LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a8
CPU := generic

ENABLE_THUMB := false

INCLUDES += \
	-I$(LOCAL_DIR)/include \
	-I$(LOCAL_DIR)/ti/include \
	-I$(LOCAL_DIR)/ti/include/hw \
	-I$(LOCAL_DIR)/ti/include/armv7a \
	-I$(LOCAL_DIR)/ti/include/armv7a/am335x \

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/ti/drivers/uart_irda_cir.c \
	$(LOCAL_DIR)/ti/drivers/dmtimer.c \
	$(LOCAL_DIR)/ti/drivers/gpio_v2.c \
	$(LOCAL_DIR)/ti/interrupt.c \

MODULES += \

MEMBASE := 0x80000000
MEMSIZE ?= 0x10000000

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

DEFINES += \
	MEMBASE=$(MEMBASE) \
	SDRAM_BASE=$(MEMBASE) \
	SDRAM_SIZE=$(MEMSIZE) \

MODULE_DEPS += \
	lib/cbuf

include make/module.mk

