LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS += $(LOCAL_DIR)/floppy.c

MODULE_DEPS += \
	dev/bus/isa \
	lib/bio

include make/module.mk
