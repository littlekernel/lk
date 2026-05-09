LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/debug.c \
	$(LOCAL_DIR)/cmdline.c

MODULE_DEPS += lib/libc

MODULE_OPTIONS := extra_warnings test

include make/module.mk
