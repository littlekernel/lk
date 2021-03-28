LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += \
	platform/bcm28xx/hvs \
	platform/bcm28xx/pixelvalve

MODULE_SRCS += \
	$(LOCAL_DIR)/dance.c

GLOBAL_INCLUDES += $(BUILDDIR)/$(LOCAL_DIR)
MODULE_CFLAGS := -O2

include make/module.mk
