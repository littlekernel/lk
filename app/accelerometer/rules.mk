LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
    $(LOCAL_DIR)/accelerometer.c \

MODULE_ARM_OVERRIDE_SRCS := \

MODULE_OPTIONS := float

include make/module.mk
