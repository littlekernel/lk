LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/debug.c
MODULE_SRCS += $(LOCAL_DIR)/fs.c
MODULE_SRCS += $(LOCAL_DIR)/shell.c

ifeq ($(call TOBOOL,$(WITH_TESTS)),true)
MODULE_DEPS += $(LOCAL_DIR)/test
endif

include make/module.mk
