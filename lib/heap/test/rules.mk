LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/heap_tests.c

MODULE_DEPS += \
    lib/heap \
    lib/unittest

include make/module.mk
