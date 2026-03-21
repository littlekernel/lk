LOCAL_DIR := $(GET_LOCAL_DIR)

# At the moment, this can only be built with hardware MMU available.
ifeq (true,$(call TOBOOL,$(WITH_KERNEL_VM)))

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/xhci-pci.cpp

MODULE_DEPS += dev/bus/pci

include make/module.mk

endif # WITH_KERNEL_VM
