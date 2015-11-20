LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/norfs \
	lib/unittest

MODULE_SRCS := \
	$(LOCAL_DIR)/norfs_test.c \
	$(LOCAL_DIR)/norfs_test_helper.c

include make/module.mk
