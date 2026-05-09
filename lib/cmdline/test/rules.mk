LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/cmdline_test.c

MODULE_DEPS += \
	lib/cmdline \
	lib/unittest

include make/module.mk

