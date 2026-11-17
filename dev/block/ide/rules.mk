LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/ide.c

MODULE_DEPS += lib/bio
MODULE_DEPS += lib/cmdline

include make/module.mk
