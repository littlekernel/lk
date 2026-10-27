LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/bio
MODULE_DEPS += lib/cksum
MODULE_DEPS += lib/cmdline
MODULE_DEPS += lib/fs
MODULE_DEPS += lib/fs/rofs
MODULE_DEPS += lib/unittest

MODULE_SRCS += $(LOCAL_DIR)/rofs_tests.c

include make/module.mk
