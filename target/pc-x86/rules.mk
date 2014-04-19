# mostly null target configuration for pc-x86
LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

PLATFORM := pc
ifneq ($(ARCH), x86-64)
MODULE_DEPS := \
	lib/ffs \

endif

MODULE_SRCS += \
	$(LOCAL_DIR)/config.c \

include make/module.mk

