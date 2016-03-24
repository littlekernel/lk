LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
	lib/iovec \
	lib/cksum

MODULE_SRCS := \
	$(LOCAL_DIR)/norfs.c

include make/module.mk
