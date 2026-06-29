LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/cmpctmalloc_tests.c

MODULE_DEPS += \
	lib/heap \
	lib/unittest

# test_cmpct_trim() uses some floating point
MODULE_OPTIONS := float

include make/module.mk
