LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/fixed_point.c

MOUDLE_OPTIONS := test

include make/module.mk
