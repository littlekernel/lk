LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := armemu
CPU := generic

WITH_KERNEL_VM := 0
KERNEL_BASE := 0x0

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/blkdev.c \
	$(LOCAL_DIR)/display.c \

#	$(LOCAL_DIR)/console.c \
	$(LOCAL_DIR)/net.c \

GLOBAL_DEFINES += \
	WITH_DEV_DISPLAY=1

MODULE_DEPS += \
	lib/gfx

MEMBASE := 0x0
MEMSIZE := 0x400000	# 4MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
