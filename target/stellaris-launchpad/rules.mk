LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

STELLARIS_CHIP := LM4F120H5QR
PLATFORM := stellaris

INCLUDES += -I$(LOCAL_DIR)/include

MODULE_SRCS += \
	$(LOCAL_DIR)/init.c

include make/module.mk
