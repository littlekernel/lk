LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH := arm
ARM_CPU := cortex-a8
#arm1136j-s
CPU := generic

DEFINES += WITH_CPU_EARLY_INIT=1 MEMBASE=0

INCLUDES += -I$(LOCAL_DIR)/include

DEVS += fbcon
MODULE_DEPS += dev/fbcon

MODULE_SRCS += \
	$(LOCAL_DIR)/arch_init.S \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/gpio.c \
	$(LOCAL_DIR)/lcdc.c

LINKER_SCRIPT += $(BUILDDIR)/system-onesegment.ld

include platform/msm_shared/rules.mk

include make/module.mk

