LOCAL_DIR := $(GET_LOCAL_DIR)

# At the moment, this can only be built with hardware MMU available.
ifeq (true,$(call TOBOOL,$(WITH_KERNEL_VM)))

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/e1000.cpp

MODULE_DEPS += dev/bus/pci
MODULE_DEPS += lib/minip

include make/module.mk

endif # WITH_KERNEL_VM
