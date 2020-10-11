LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/sdhost_impl.cpp

MODULES += lib/bio

include make/module.mk
