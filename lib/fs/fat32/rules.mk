LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/fs \
	lib/bcache \
	lib/bio

MODULE_SRCS += $(LOCAL_DIR)/fat.cpp
MODULE_SRCS += $(LOCAL_DIR)/file.cpp

include make/module.mk
