LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/fs
MODULE_DEPS += lib/unittest

MODULE_SRCS += $(LOCAL_DIR)/test.c

include make/module.mk
