LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm64
ARM64_CPU := emu

GLOBAL_DEFINES += \
	PLATFORM_HAS_DYNAMIC_TIMER=1

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/semihost.S

# first 2GB of ram
MEMBASE := 0x80000000
MEMSIZE := 0x80000000

GLOBAL_DEFINES += \
	MEMBASE=$(MEMBASE) \
	MEMSIZE=$(MEMSIZE)

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
