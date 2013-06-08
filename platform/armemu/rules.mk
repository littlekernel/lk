LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := arm926ej-s
CPU := generic

# emulater doesn't support thumb properly
ENABLE_THUMB := false

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

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

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
