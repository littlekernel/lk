LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

SAM_CHIP := sam3x8h
PLATFORM := sam3

INCLUDES += -I$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk

