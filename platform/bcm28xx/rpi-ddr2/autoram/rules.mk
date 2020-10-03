LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_DEPS += platform/bcm28xx/rpi-ddr2

MODULE_SRCS += $(LOCAL_DIR)/autoram.c

GLOBAL_DEFINES += NOVM_MAX_ARENAS=2 NOVM_DEFAULT_ARENA=0

include make/module.mk
