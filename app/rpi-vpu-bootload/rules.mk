LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += platform/bcm28xx/rpi-ddr2

MODULE_SRCS += $(LOCAL_DIR)/bootloader.c

include make/module.mk
