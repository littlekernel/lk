LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/fixed_point_tests.c
MODULE_SRCS += $(LOCAL_DIR)/fixed_point_snprintf_tests.c

MODULE_DEPS += \
	lib/fixed_point \
	lib/unittest

include make/module.mk