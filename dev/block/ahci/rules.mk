LOCAL_DIR := $(GET_LOCAL_DIR)

# At the moment, this can only be built with hardware MMU available.
ifeq (true,$(call TOBOOL,$(WITH_KERNEL_VM)))

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/ahci.cpp
MODULE_SRCS += $(LOCAL_DIR)/ata.cpp
MODULE_SRCS += $(LOCAL_DIR)/disk.cpp
MODULE_SRCS += $(LOCAL_DIR)/port.cpp

MODULE_DEPS += dev/bus/pci
MODULE_DEPS += lib/bio

MODULE_CPPFLAGS += -Wno-invalid-offsetof

include make/module.mk

endif # WITH_KERNEL_VM
