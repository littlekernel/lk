LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := arm1136j-s
CPU := generic

INCLUDES += -I$(LOCAL_DIR)/include

MODULE_DEPS := dev/fbcon

MODULE_SRCS := \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/mddi.c \
	$(LOCAL_DIR)/gpio.c 

LINKER_SCRIPT := $(BUILDDIR)/system-onesegment.ld

include platform/msm_shared/rules.mk

include make/module.mk
