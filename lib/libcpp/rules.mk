LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS :=

MODULE_SRCS += $(LOCAL_DIR)/abort.cpp
MODULE_SRCS += $(LOCAL_DIR)/new.cpp
MODULE_SRCS += $(LOCAL_DIR)/pure_virtual.cpp

# build the tests in test/ when WITH_TESTS is enabled
MODULE_OPTIONS := extra_warnings test

include make/module.mk
