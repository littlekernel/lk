LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/system_nrf51.c \


include make/module.mk

