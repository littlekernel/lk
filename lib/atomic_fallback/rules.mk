LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/atomic_fallback.c

MODULE_OPTIONS := extra_warnings test

include make/module.mk
