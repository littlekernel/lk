LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a9-neon
WITH_SMP ?= 1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/uart.c

MEMBASE := 0x60000000
MEMSIZE := 0x20000000	# 512MB

MODULE_DEPS += \
	lib/cbuf \
	dev/interrupt/arm_gic \
	dev/timer/arm_cortex_a9 \
	dev/virtio/block \
	dev/virtio/net

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE) \
	ARM_ARCH_WAIT_FOR_SECONDARIES=1

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
