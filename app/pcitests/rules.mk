LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += \
	$(LOCAL_DIR)/pci_tests.c

MODULE_DEPS += dev/bus/pci

include make/module.mk
