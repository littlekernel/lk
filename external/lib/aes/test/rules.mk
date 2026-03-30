# aes implementation from http://students.cs.byu.edu/~cs465ta/labs/AESImplementation.html
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/aes_test.c

MODULE_DEPS += \
	lib/unittest

include make/module.mk
