LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a8
CPU := generic

# provides a few devices
GLOBAL_DEFINES += \
	WITH_DEV_UART=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/cpu_early_init.S \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/i2c.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/uart.c \

#	$(LOCAL_DIR)/usbc.c

MEMBASE := 0x80000000

GLOBAL_DEFINES += MEMBASE=$(MEMBASE) \
	WITH_CPU_EARLY_INIT=1

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
