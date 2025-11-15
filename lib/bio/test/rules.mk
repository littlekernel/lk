LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/bio_tests.c

MODULE_DEPS += \
	lib/bio \
	lib/unittest

include make/module.mk
