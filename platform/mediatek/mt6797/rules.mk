LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

ARCH    := arm
ARM_CPU := cortex-a7
CPU     := generic
WITH_SMP ?= 0

GLOBAL_INCLUDES += \
	$(LOCAL_DIR)/$(SUB_PLATFORM)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/platform.c \
	$(LOCAL_DIR)/uart.c \
	$(LOCAL_DIR)/interrupts.c \
	$(LOCAL_DIR)/timer.c \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/mt_gpt.c

KERNEL_BASE = $(MEMBASE)

include platform/mediatek/common/rules.mk

include make/module.mk

