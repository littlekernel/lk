LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/spifs.c

MODULE_DEPS += lib/bio
MODULE_DEPS += lib/cksum
MODULE_DEPS += lib/fs

MODULE_OPTIONS := test

include make/module.mk
