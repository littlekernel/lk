LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)
MODULE_OPTIONS := test extra_warnings

MODULE_SRCS += $(LOCAL_DIR)/pool.cpp

include make/module.mk
