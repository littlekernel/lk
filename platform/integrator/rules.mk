LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := arm926ej-s
CPU := generic

INCLUDES += \
	-I$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/timer.c \

#	$(LOCAL_DIR)/net.c \


#	$(LOCAL_DIR)/console.c \

MEMBASE ?= 0x0
MEMSIZE ?= 0x08000000	# 128MB

LINKER_SCRIPT += \
	$(BUILDDIR)/system-onesegment.ld

include make/module.mk
