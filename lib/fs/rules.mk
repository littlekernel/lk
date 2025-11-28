LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/debug.c
MODULE_SRCS += $(LOCAL_DIR)/fs.c
MODULE_SRCS += $(LOCAL_DIR)/shell.c

MODULE_OPTIONS := test

include make/module.mk
