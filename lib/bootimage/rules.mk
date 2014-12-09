LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

GLOBAL_INCLUDES += $(LOCAL_DIR)/include

MODULE_DEPS := \
    lib/mincrypt

MODULE_SRCS := \
	$(LOCAL_DIR)/bootimage.c

include make/module.mk
