LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/bcache
MODULE_DEPS += lib/bio
MODULE_DEPS += lib/fs
MODULE_DEPS += lib/libcpp

MODULE_SRCS += $(LOCAL_DIR)/dir.cpp
MODULE_SRCS += $(LOCAL_DIR)/fat.cpp
MODULE_SRCS += $(LOCAL_DIR)/file.cpp
MODULE_SRCS += $(LOCAL_DIR)/fs.cpp

include make/module.mk
