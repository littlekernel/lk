LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS := lib/fdt

MODULE_SRCS := $(LOCAL_DIR)/fdtwalk.cpp
MODULE_SRCS += $(LOCAL_DIR)/helpers.cpp

MODULE_WEAK_DEPS += \
	dev/bus/pci \
	dev/interrupt/arm_gic \
	dev/power/psci \
	lib/cmdline

include make/module.mk
