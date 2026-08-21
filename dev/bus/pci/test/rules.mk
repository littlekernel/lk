LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/resource_tests.cpp

MODULE_DEPS += \
    dev/bus/pci \
    lib/libcpp \
    lib/unittest

include make/module.mk
