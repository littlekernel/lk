LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/e1000.cpp

MODULE_DEPS += dev/bus/pci
MODULE_DEPS += lib/minip

include make/module.mk
