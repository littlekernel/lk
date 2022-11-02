LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_OPTIONS := extra_warnings

MODULE_DEPS := \
	lib/libcpp

MODULE_SRCS += \
   $(LOCAL_DIR)/acpi_lite.cpp

include make/module.mk
