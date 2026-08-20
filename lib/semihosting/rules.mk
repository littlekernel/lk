LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/semihosting.c

MODULE_DEPS += \
	lib/libc

MODULE_OPTIONS := extra_warnings

include make/module.mk
