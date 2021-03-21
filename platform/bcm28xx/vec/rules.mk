LOCAL_DIR := $(GET_LOCAL_DIR)
MODULE := $(LOCAL_DIR)
MODULE_SRCS += $(LOCAL_DIR)/vec.c

MODULES += platform/bcm28xx/pixelvalve platform/bcm28xx/hvs lib/tga platform/bcm28xx/power

include make/module.mk
