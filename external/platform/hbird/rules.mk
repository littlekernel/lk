LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/inc

MODULE_SRCS += \
	$(LOCAL_DIR)/src/hbird_common.c \
	$(LOCAL_DIR)/src/hbird_gpio.c \
	$(LOCAL_DIR)/src/hbird_uart.c \
	$(LOCAL_DIR)/src/system_hbird.c

include $(LOCAL_DIR)/NMSIS/rules.mk

include make/module.mk
