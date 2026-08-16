LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/pcnet.c

MODULE_DEPS := \
	dev/bus/pci \
	lib/minip

include make/module.mk
