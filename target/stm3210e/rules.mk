LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

STM32_CHIP := stm32f103_hd

PLATFORM := stm32f1xx

INCLUDES += -I$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk

