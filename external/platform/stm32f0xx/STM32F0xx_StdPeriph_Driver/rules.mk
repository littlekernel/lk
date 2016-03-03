LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += \
	$(LOCAL_DIR)/src/system_stm32f0xx.c

include $(LOCAL_DIR)/CMSIS/rules.mk

include make/module.mk
