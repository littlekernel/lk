LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := arm926ej-s
CPU := generic

MODULE_DEPS += \
	lib/cbuf

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/timer.c


#	$(LOCAL_DIR)/console.c \

MEMBASE := 0x10000000
#MEMSIZE := 0x02000000	# 32MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
