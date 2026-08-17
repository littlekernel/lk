LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/timer_tests.c \

MODULE_DEPS += \
	lib/unittest

include make/module.mk
