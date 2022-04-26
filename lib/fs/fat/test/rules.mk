LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/fs/fat

MODULE_SRCS += $(LOCAL_DIR)/test.cpp

include make/module.mk
