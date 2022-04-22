LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	lib/fs \
	lib/bcache \
	lib/bio

MODULE_SRCS += $(LOCAL_DIR)/fs.cpp
MODULE_SRCS += $(LOCAL_DIR)/fat.cpp
MODULE_SRCS += $(LOCAL_DIR)/file.cpp
MODULE_SRCS += $(LOCAL_DIR)/dir.cpp

include make/module.mk
