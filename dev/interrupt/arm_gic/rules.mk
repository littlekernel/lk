LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/arm_gic.c
MODULE_SRCS += $(LOCAL_DIR)/gic_v2.c
MODULE_SRCS += $(LOCAL_DIR)/gic_v3.c

MODULE_OPTIONS += extra_warnings

include make/module.mk
