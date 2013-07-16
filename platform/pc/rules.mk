LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := x86
CPU := generic

MODULE_DEPS += \
	lib/cbuf \
	lib/lwip \

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/console.c \
	$(LOCAL_DIR)/keyboard.c \
	$(LOCAL_DIR)/pci.c \
	$(LOCAL_DIR)/ide.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/pcnet.c \

LINKER_SCRIPT += \
	$(BUILDDIR)/kernel.ld

include make/module.mk

