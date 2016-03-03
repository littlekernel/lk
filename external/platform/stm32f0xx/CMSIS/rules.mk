LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += $(LOCAL_DIR)/system_stm32f0xx.c

include make/module.mk
