LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_OPTIONS := extra_warnings

MODULE_DEPS := external/lib/uacpi

MODULE_SRCS += $(LOCAL_DIR)/acpi.c

include make/module.mk
