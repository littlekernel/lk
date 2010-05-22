LOCAL_DIR := $(GET_LOCAL_DIR)

ARCH := arm
ARM_CPU := arm926ej-s
CPU := generic

# emulater doesn't support thumb properly
ENABLE_THUMB := false

INCLUDES += \
	-I$(LOCAL_DIR)/include

OBJS += \
	$(LOCAL_DIR)/debug.o \
	$(LOCAL_DIR)/interrupts.o \
	$(LOCAL_DIR)/platform.o \
	$(LOCAL_DIR)/timer.o \
	$(LOCAL_DIR)/blkdev.o \
	$(LOCAL_DIR)/display.o \


#	$(LOCAL_DIR)/console.o \
	$(LOCAL_DIR)/net.o \

DEFINES += \
	WITH_DEV_DISPLAY=1

MODULES += \
	lib/gfx


MEMBASE := 0x0
MEMSIZE := 0x400000	# 4MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

