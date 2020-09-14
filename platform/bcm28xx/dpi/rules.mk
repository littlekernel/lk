LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULES += \
	platform/bcm28xx/pixelvalve \
	platform/bcm28xx/hvs \


MODULE_SRCS += \
	$(LOCAL_DIR)/dpi.c \

include make/module.mk
