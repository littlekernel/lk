LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := lib/fs

MODULE_SRCS += \
	$(LOCAL_DIR)/memfs.c

include make/module.mk
