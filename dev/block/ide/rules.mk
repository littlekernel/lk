LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/ide.c

MODULE_DEPS += lib/bio

MODULE_WEAK_DEPS += dev/bus/pci

include make/module.mk
