LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/fs/9p
MODULE_DEPS += lib/unittest

MODULE_SRCS += $(LOCAL_DIR)/v9fs_tests.c
MODULE_SRCS += $(LOCAL_DIR)/v9p_tests.c

include make/module.mk
