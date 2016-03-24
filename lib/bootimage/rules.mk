LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := \
    lib/mincrypt

MODULE_SRCS := \
	$(LOCAL_DIR)/bootimage.c

include make/module.mk
