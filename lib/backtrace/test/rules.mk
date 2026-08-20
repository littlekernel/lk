LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/backtrace_tests.c

MODULE_DEPS += \
    lib/backtrace \
    lib/unittest

include make/module.mk
