LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_OPTIONS := extra_warnings

MODULE_SRCS += $(LOCAL_DIR)/pool_test.cpp

MODULE_DEPS += lib/unittest

include make/module.mk
