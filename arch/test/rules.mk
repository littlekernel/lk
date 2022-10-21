LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := $(LOCAL_DIR)/mmu.cpp

MODULE_DEPS := lib/libcpp
MODULE_DEPS += lib/unittest

include make/module.mk

