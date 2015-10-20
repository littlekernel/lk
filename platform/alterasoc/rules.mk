LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a9-neon
WITH_SMP := 1

MODULE_DEPS := \
	lib/cbuf \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9

MODULE_SRCS += \
	$(LOCAL_DIR)/clocks.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/uart.c

MEMBASE := 0x0
MEMSIZE ?= 0x10000000	# 256MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
