LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	platform/bcm28xx/hvs

MODULE_SRCS += \
	$(LOCAL_DIR)/dance.c

include make/module.mk
