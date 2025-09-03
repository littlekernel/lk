LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/spifs.c

ifeq ($(call TOBOOL,$(WITH_TESTS)),true)
MODULE_DEPS += $(LOCAL_DIR)/test
endif

MODULE_DEPS += lib/bio
MODULE_DEPS += lib/cksum
MODULE_DEPS += lib/fs

include make/module.mk
