LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS :=

MODULE_SRCS += $(LOCAL_DIR)/new.cpp
MODULE_SRCS += $(LOCAL_DIR)/pure_virtual.cpp

include make/module.mk
