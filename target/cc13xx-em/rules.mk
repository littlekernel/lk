LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := cc13xx

MODULE_SRCS := \
	$(LOCAL_DIR)/init.c

include make/module.mk
