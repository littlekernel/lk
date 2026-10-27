LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += lib/bio
MODULE_DEPS += lib/fs

MODULE_SRCS += $(LOCAL_DIR)/rofs.c

MODULE_OPTIONS := test

include make/module.mk
